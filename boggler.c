/*

A boggle solver,
by Allen Webster

03.07.2016 (dd.mm.yyyy)

How to use:
Call the application with two text files.

The first should be a dictionary containing one
word per line.  This system will check to make
sure the dictionary is sorted, but if it isn't
the sorting is not particularly good right now.
The dictionary is not provided but they are not
hard to find with a google search.

The second text file is the boggle board. It should
be a 4 by 4 matrix of capital letters with no spaces
and newlines only at the end of each row.  Here is
a particularly interesting board I rolled for testing:

EYRL
OQOS
VWET
LYUS

TODO(allen): 
Better sorting
Add break down commands for querying:
-All the words of a particular length
-The number of words that start on each grid position
-The number of words that use each grid position
-The number of ways to form each word
Options for:
-Output sorted by length
-Output sorted alphabetically
*/

// TOP

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct Data{
    char *data;
    int32_t size;
};

static struct Data
file_dump(char *filename){
    struct Data data = {0};
    FILE *file = fopen(filename, "rb");
    
    if (file){
        fseek(file, 0, SEEK_END);
        data.size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        data.data = (char*)malloc(data.size+1);
        fread(data.data, 1, data.size, file);
        data.data[data.size] = 0;
        
        fclose(file);
    }
    
    return(data);
}

#define BOGGLE_BOARD_W 4
#define BOGGLE_BOARD_H 4
#define BOGGLE_BOARD_SIZE BOGGLE_BOARD_W*BOGGLE_BOARD_H

struct Boggle_Board{
    char letter[BOGGLE_BOARD_SIZE];
};

static int32_t
is_letter(char c){
    int32_t result = 0;
    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z')){
        result = 1;
    }
    return(result);
}

static char
to_upper(char c){
    if (c >= 'a' && c <= 'z'){
        c += 'A' - 'a';
    }
    return(c);
}

struct Integer_Pos{
    int32_t x, y;
};

#define BPOS(x,y) {x, y}

static struct Integer_Pos moore[] = {
    BPOS(-1, -1),
    BPOS( 0, -1),
    BPOS( 1, -1),
    BPOS( 1,  0),
    BPOS( 1,  1),
    BPOS( 0,  1),
    BPOS(-1,  1),
    BPOS(-1,  0)
};

static struct Integer_Pos
pos_add(struct Integer_Pos a, struct Integer_Pos b){
    struct Integer_Pos r = {0};
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return(r);
}

static int32_t
pos_i(struct Integer_Pos p){
    int32_t i = (p.x + p.y*BOGGLE_BOARD_W);
    return(i);
}

struct Visit_Record{
    int32_t visit[BOGGLE_BOARD_W * BOGGLE_BOARD_H];
};

static struct Visit_Record
make_visit_record(){
    struct Visit_Record visit_record = {0};
    return(visit_record);
}

struct Dictionary{
    char **word;
    int32_t *len;
    int32_t *already_used;
    int32_t count;
};

static struct Dictionary
make_dictionary(int32_t word_count){
    struct Dictionary dict = {0};
    int32_t mem_size = (word_count*(sizeof(char*) + sizeof(int)*2));
    dict.word = (char**)malloc(mem_size);
    dict.len = (int*)(dict.word + word_count);
    dict.already_used = (int*)(dict.len + word_count);
    memset(dict.word, 0, mem_size);
    dict.count = 0;
    return(dict);
}

#define Swap(t,a,b) do { t T = a; a = b; b = T; } while (0)

static void
bubble_sort(struct Dictionary dict){
    int32_t start = 0;
    int32_t end = dict.count-1;
    int32_t last_swap = -1;
    do{
        last_swap = -1;
        for (int32_t i = start;
             i < end;
             ++i){
            if (strcmp(dict.word[i], dict.word[i+1]) > 0){
                Swap(int32_t, dict.len[i], dict.len[i+1]);
                Swap(char*, dict.word[i], dict.word[i+1]);
                last_swap = i;
            }
        }
        end = last_swap;
    }while(last_swap != -1);
}

struct Match_Range{
    int32_t first;
    int32_t last;
    int32_t index;
};

struct Match_Range
make_range(struct Dictionary dict){
    struct Match_Range range;
    range.first = 0;
    range.last = dict.count;
    range.index = 0;
    return(range);
}

struct Match_Range
narrow_range(struct Match_Range range,
             struct Dictionary dict,
             char letter){
    int32_t str_index = range.index;
    
    int32_t s = range.first;
    int32_t e = range.last;
    
    int32_t new_s = range.first;
    int32_t new_e = range.last;
    int32_t i = 0;
    
    if (s != e){
        for(;s < e;){
            i = (s+e)/2;
            
            if (i > range.first){
                if (dict.len[i-1] > str_index){
                    if (dict.word[i-1][str_index] >= letter){
                        e = i;
                        continue;
                    }
                }
            }
            
            if (dict.len[i] > str_index){
                if (dict.word[i][str_index] >= letter){
                    break;
                }
            }
            
            s = i+1;
            continue;
        }
        
        new_s = i;
        
        if (s >= e){
            new_e = new_s;
        }
        else{
            s = new_s;
            e = range.last + 1;
            
            for(;s < e;){
                i = (s+e)/2;
                
                if (i > range.first){
                    if (dict.len[i-1] > str_index){
                        if (dict.word[i-1][str_index] > letter){
                            e = i;
                            continue;
                        }
                    }
                }
                
                if (dict.len[i] > str_index){
                    if (dict.word[i][str_index] > letter){
                        break;
                    }
                }
                
                s = i+1;
                continue;
            }
            
            new_e = i;
        }
    }
    
    struct Match_Range new_range;
    new_range.first = new_s;
    new_range.last = new_e;
    new_range.index = str_index + 1;
    return(new_range);
}

// NOTE(allen): Boggle has no 'Q' but it has a die with a 'Qu'.
static struct Match_Range
narrow_range_boggle(struct Match_Range range,
                    struct Dictionary dict,
                    char letter){
    struct Match_Range new_range = narrow_range(range, dict, letter);
    if (letter == 'Q'){
        new_range = narrow_range(new_range, dict, 'U');
    }
    return(new_range);
}

static int32_t
can_visit_pos(struct Visit_Record *visit_record, struct Integer_Pos pos){
    int32_t result = 0;
    if (pos.x >= 0 && pos.x < BOGGLE_BOARD_W &&
        pos.y >= 0 && pos.y < BOGGLE_BOARD_H &&
        visit_record->visit[pos_i(pos)] == 0){
        result = 1;
    }
    return(result);
}

struct Solution{
    char **words;
    int32_t word_count;
    int32_t total_score;
};

static void
explore_neighborhood(struct Dictionary dict, struct Match_Range range,
                     struct Boggle_Board board, struct Visit_Record *visit_record,
                     struct Integer_Pos pos, struct Solution *solution){
    if (dict.len[range.first] == range.index){
        if (dict.already_used[range.first] == 0){
            dict.already_used[range.first] = 1;
            
            solution->words[solution->word_count++] = dict.word[range.first];
            
            int32_t score = 0;
            if (range.index == 3){
                score = 1;
            }
            else if (range.index > 3){
                score = range.index - 3;
            }
            
            solution->total_score += score;
        }
    }
    
    int32_t count = (sizeof(moore)/sizeof(*moore));
    for (int32_t i = 0;
         i < count;
         ++i){
        struct Integer_Pos relative_pos = pos_add(pos, moore[i]);
        if (can_visit_pos(visit_record, relative_pos)){
            char letter = board.letter[pos_i(relative_pos)];
            
            struct Match_Range new_range = narrow_range_boggle(range, dict, letter);
            
            if (new_range.first < new_range.last){
                visit_record->visit[pos_i(relative_pos)] = 1;
                explore_neighborhood(dict, new_range, board,
                                     visit_record, relative_pos, solution);
                visit_record->visit[pos_i(relative_pos)] = 0;
            }
        }
    }
}
                     

int main(int argc, char **argv){
    if (argc != 3){
        fprintf(stderr, "usaged:\n\t%s <dictionary> <board>\n", argv[0]);
        exit(1);
    }
    
    struct Data dictionary_file = file_dump(argv[1]);
    struct Data board_file = file_dump(argv[2]);
    
    struct Boggle_Board board = {0};
    int32_t board_error = 0;
    
    {
        int32_t row = 0;
        int32_t col = 0;
        for (int32_t i = 0;
             i < board_file.size;
             ++i){
            char c = board_file.data[i];
            if (is_letter(c)){
                if (col < 4){
                    if (row < 4){
                        board.letter[col + row*BOGGLE_BOARD_W] = to_upper(c);
                        ++col;
                    }
                    else{
                        board_error = 1;
                        break;
                    }
                }
                else{
                    board_error = 1;
                    break;
                }
            }
            else if (c == '\r'){
                // NOTE(allen): do nothing
            }
            else if (c == '\n'){
                if (col == 4){
                    col = 0;
                    ++row;
                }
                else{
                    board_error = 1;
                    break;
                }
            }
            else{
                board_error = 1;
                break;
            }
        }
    }
    
    if (board_error){
        fprintf(stderr, "error in boggle board format\n");
        exit(1);
    }
    
    int32_t line_count = 1;
    
    for (int32_t i = 0;
         i < dictionary_file.size;
         ++i){
        if (dictionary_file.data[i] == '\n'){
            ++line_count;
        }
    }
    
    struct Dictionary dict = make_dictionary(line_count);
    
    for (int32_t i = 0;
         i < dictionary_file.size;
         ++i){
        
        int32_t word_start = i;
        
        for (; i < dictionary_file.size;
             ++i){
            if (dictionary_file.data[i] == '\n'){
                break;
            }
        }
        
        int32_t word_end = i;
        
        int32_t early_end = word_end;
        int32_t good_word = 1;
        for (int32_t j = word_start;
             j < word_end;
             ++j){
            char c = dictionary_file.data[j];
            
            if (is_letter(c)){
                if (early_end != word_end){
                    good_word = 0;
                    break;
                }
                dictionary_file.data[j] = to_upper(c);
            }
            else{
                early_end = j;
            }
        }
        
        if (good_word && (early_end - word_start) > 2){
            dictionary_file.data[early_end] = 0;
            dict.len[dict.count] = (early_end - word_start);
            dict.word[dict.count] = dictionary_file.data + word_start;
            ++dict.count;
        }
    }
    
    bubble_sort(dict);
    
    struct Solution solution = {0};
    solution.words = (char**)malloc(sizeof(char*)*dict.count);
    
    struct Visit_Record visit_record = make_visit_record();
    struct Integer_Pos pos = {0};
    
    for (pos.y = 0;
         pos.y < BOGGLE_BOARD_H;
         ++pos.y){
        for (pos.x = 0;
             pos.x < BOGGLE_BOARD_H;
             ++pos.x){
            
            struct Match_Range range = make_range(dict);
            
            visit_record.visit[pos_i(pos)] = 1;
            range = narrow_range_boggle(range, dict, board.letter[pos_i(pos)]);
            explore_neighborhood(dict, range, board, &visit_record, pos, &solution);
            visit_record.visit[pos_i(pos)] = 0;
        }
    }
    
    for (int32_t i = 0;
         i < solution.word_count;
         ++i){
        printf("%s\n", solution.words[i]);
    }
    printf("word count:  %d\n", solution.word_count);
    printf("total score: %d\n", solution.total_score);
    
    return(0);
}

// BOTTOM



