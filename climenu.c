#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>


/* Key codes */
#define KEY_J       0x6A
#define KEY_K       0x6B
#define KEY_DOWN    0x425B1B
#define KEY_UP      0x415B1B
#define KEY_ENTER   0xD
#define KEY_SPACE   0x20
#define KEY_Q       0x71
#define KEY_CTRLC   0x03

#define DRAW_CENTERED   0


enum EntryType {
    ET_NONE     = 0,
    ET_REG      = 1,
    ET_HEADER   = 2
};


struct EntryData_Reg {
    char* str;
    char* exec;
    int   wait;
};


struct EntryData_Header {
    char* str;
};


struct Entry {
    struct Entry* next;
    struct Entry* prev;
    void* data;
    char  type;
};


struct Entry*  g_selected = NULL;
struct termios g_termios_original;


void error_exit(char* msg) {
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}


size_t get_filesize(FILE* file_ptr) {
    size_t size;

    fseek(file_ptr, 0, SEEK_END);
    size = ftell(file_ptr);
    rewind(file_ptr);

    return size;
}


char* read_file(char* path) {
    FILE*  file_ptr;
    size_t file_size;
    char*  buf;
    
    file_ptr = fopen(path, "r");
    
    if (file_ptr == NULL)
        return NULL;
    
    file_size = get_filesize(file_ptr);
    buf = calloc(1, file_size + 1);

    if (buf == NULL) {
        fclose(file_ptr);
        return NULL;
    }    

    fread(buf, 1, file_size, file_ptr);
    fclose(file_ptr);
    return buf;
}


int string_startswith(char* a, char* b) {
    while (*a) {
        if (*b == '\0')
            return 1;

        if (*(a++) != *(b++))
            return 0;
    }
    
    return 1;
}


char* string_dup(char* str) {
    char* new_str;
    
    new_str = calloc(1, strlen(str) + 1);

    if (new_str == NULL)
        error_exit("calloc returned NULL");

    strcpy(new_str, str);
    return new_str;
}


char* next_line(char* ptr) {
    while (*ptr && *ptr != '\n')
        ++ptr;
    
    if (*ptr == '\n')
        ++ptr;
    else
        return NULL;

    return ptr;
}


char* get_value(char* ptr) {
    char buf[BUFSIZ] = {0};
    
    while (*ptr && *ptr != '=')
        ++ptr;
    
    if (*ptr)
        ++ptr;
    
    for (int i = 0; i < BUFSIZ - 1 && *ptr && *ptr != '\n'; ++i)
        buf[i] = *(ptr++);
    
    return string_dup(buf);
}


struct Entry* entry_create(enum EntryType type) {
    struct Entry* new_entry;
    
    new_entry = malloc(sizeof *new_entry);
    new_entry->type = type;
    new_entry->next = NULL;
    new_entry->prev = NULL;

    return new_entry;
}


void entry_append(struct Entry* head, struct Entry* node) {
    while (head->next)
        head = head->next;
    
    head->next = node;
    node->prev = head;
}


void entry_destroy(struct Entry* entry) {
    if (entry->type == ET_REG) {
        struct EntryData_Reg* data = entry->data;
        free(data->exec);
        free(data->str);
    }

    if (entry->type == ET_HEADER) {
        struct EntryData_Header* data = entry->data;
        free(data->str);
    }

    free(entry->data);
    free(entry);
}


struct Entry* parse_entry_reg(char* ptr) {
    struct Entry* entry;
    struct EntryData_Reg* entry_data;

    entry = entry_create(ET_REG);
    entry_data = malloc(sizeof *entry_data);
    
    entry_data->str = NULL;
    entry_data->exec = NULL;
    entry_data->wait = 0;

    for (int i = 0; i < 3; ++i) {
        ptr = next_line(ptr);
        
        if (ptr == NULL)
            break;
            
        if (string_startswith(ptr, "str=") && entry_data->str == NULL)
            entry_data->str = get_value(ptr);
            
        if (string_startswith(ptr, "exec=") && entry_data->exec == NULL)
            entry_data->exec = get_value(ptr);

        if (string_startswith(ptr, "wait=") && entry_data->wait == 0) {
            char* wait_val = get_value(ptr);
            entry_data->wait = strcmp(wait_val, "true") == 0;
            free(wait_val);
        }
    }

    entry->data = entry_data;
    return entry;
}


struct Entry* parse_entry_header(char* ptr) {
    struct Entry* entry;
    struct EntryData_Header* entry_data;

    entry = entry_create(ET_HEADER);
    entry_data = malloc(sizeof *entry_data);
    
    ptr = next_line(ptr);

    if (string_startswith(ptr, "str="))
        entry_data->str = get_value(ptr);

    entry->data = entry_data;
    return entry;
}


struct Entry* parse_entries(char* entries_path) {
    struct Entry* entry_head = NULL;

    char* file_content;
    char* ptr;

    file_content = read_file(entries_path);
    
    if (file_content == NULL)
        error_exit("Failed to read entries file");
    
    ptr = file_content;

    while (*ptr) {
        struct Entry* entry_curr = NULL;

        if (string_startswith(ptr, "[Entry]")) 
            entry_curr = parse_entry_reg(ptr);

        if (string_startswith(ptr, "[Header]")) 
            entry_curr = parse_entry_header(ptr);
        
        if (entry_curr) {
            if (entry_head == NULL) {
                entry_head = entry_curr;
            } else {
                entry_append(entry_head, entry_curr);
            }
        }
        
        ptr = next_line(ptr);
        
        if (ptr == NULL)
            break;
    }
    
    free(file_content);
    return entry_head;
}


void select_entry(struct Entry* entry) {
    if (entry->type != ET_REG)
        select_entry(entry->next);
    else
        g_selected = entry;
}


void select_next(void) {
    if (g_selected->next)
        g_selected = g_selected->next;

    if (g_selected->type != ET_REG)
        select_next();
}


void select_prev(void) {
    if (g_selected->prev) {
        g_selected = g_selected->prev;
        
        if (g_selected->type != ET_REG) {
            select_prev();
            
            if (g_selected->type != ET_REG)
                select_next();
        }
    }
}


void cursor_pos(int x, int y) {
    printf("\033[%d;%df", y, x);
}


void cursor_visible(int vis) {
    if (vis)
        printf("\033[?25h");
    else
        printf("\033[?25l");
}


void draw(struct Entry* head) {
    struct winsize w;
    struct Entry* cursor = head;
    
    int x;
    int y;

    ioctl(0, TIOCGWINSZ, &w);
    
    if (DRAW_CENTERED) {
        x = (w.ws_col / 2);
        y = (w.ws_row / 2);
    } else {
        x = 0;
        y = 0;
    }

    cursor_visible(0);
    
    while (cursor) {
        cursor_pos(x, ++y);
        
        if (cursor->type == ET_HEADER) {
            struct EntryData_Header* data = cursor->data;
            printf("\033[4m%s\033[24m", data->str);
        }
        
        if (cursor->type == ET_REG) {
            struct EntryData_Reg* data = cursor->data;

            if (cursor == g_selected)
                printf("%s (*)", data->str);
            else
                printf("%s       ", data->str);
        }
        
        cursor = cursor->next;
    }

    fflush(stdout);
}


void restore_terminal_mode(void) {
    tcsetattr(0, TCSANOW, &g_termios_original);
}


void set_terminal_mode(void) {
    struct termios termios_new;

    tcgetattr(0, &g_termios_original);
    memcpy(&termios_new, &g_termios_original, sizeof termios_new);

    atexit(restore_terminal_mode);
    cfmakeraw(&termios_new);
    tcsetattr(0, TCSANOW, &termios_new);
}


uint32_t getch(void) {
    char buf[4] = {0};
    if (read(0, buf, 4) != -1) {
        /* A 32 bit int is required */
        return *(uint32_t*) buf;   
    } else {
        return 0;
    }
}


int main(int argc, char** argv) {
    struct Entry* head;

    if (argc > 1)
        head = parse_entries(argv[1]);
    else
        error_exit("No entries file provided");

    select_entry(head);
    set_terminal_mode();
    system("clear");

    while (1) {
        draw(head);

        switch (getch())
        {
            case KEY_K:
            case KEY_UP:
                select_prev();
                break;
            
            case KEY_J:
            case KEY_DOWN:
                select_next();
                break;
            
            case KEY_SPACE:
            case KEY_ENTER:;
                struct EntryData_Reg* data = g_selected->data;

                restore_terminal_mode();
                system("clear");
                system(data->exec);
                
                if (data->wait) {
                    set_terminal_mode();
                    printf("Press any key to continue...\n");
                    getch();
                } else {
                    set_terminal_mode();
                }

                system("clear");

                break;

            case KEY_Q:
            case KEY_CTRLC:
                goto cleanup;

            default:
                break;
        }
    }
    

cleanup:;
    while (head->next) {
        head = head->next;
        entry_destroy(head->prev);
    }

    entry_destroy(head);

    restore_terminal_mode();
    cursor_pos(0, 0);
    cursor_visible(1);
    system("clear");
    return EXIT_SUCCESS;
}
