#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>


#define COLOR_DEFAULT ""
#define COLOR_RESET      "\033[0m"

#define COLOR_FG_BLACK   "\033[30m"
#define COLOR_FG_RED     "\033[31m"
#define COLOR_FG_GREEN   "\033[32m"
#define COLOR_FG_YELLOW  "\033[33m"
#define COLOR_FG_BLUE    "\033[34m"
#define COLOR_FG_MAGENTA "\033[35m"
#define COLOR_FG_CYAN    "\033[36m"
#define COLOR_FG_WHITE   "\033[37m"

#define COLOR_BG_BLACK   "\033[40m"
#define COLOR_BG_RED     "\033[41m"
#define COLOR_BG_GREEN   "\033[42m"
#define COLOR_BG_YELLOW  "\033[43m"
#define COLOR_BG_BLUE    "\033[44m"
#define COLOR_BG_MAGENTA "\033[45m"
#define COLOR_BG_CYAN    "\033[46m"
#define COLOR_BG_WHITE   "\033[47m"


/* Key codes */
#define KEY_J       0x6A
#define KEY_K       0x6B
#define KEY_DOWN    0x425B1B
#define KEY_UP      0x415B1B
#define KEY_ENTER   0xD
#define KEY_SPACE   0x20
#define KEY_Q       0x71
#define KEY_CTRLC   0x03


enum ColorMode {
    CM_NONE     = 0,
    CM_SELECTED
};


enum EntryType {
    ET_NONE     = 0,
    ET_REG,
    ET_HEADER
};


struct EntryText {
    char* str;
    char* fgcolor;
    char* bgcolor;
};


struct EntryData_Reg {
    char* exec;
    int   wait;
    int   colormode;
};


struct Entry {
    struct Entry* next;
    struct Entry* prev;
    struct EntryText* text;
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
    while (*ptr && *(ptr++) != '\n')
        ;

    return (*ptr) ? ptr : NULL;
}


char* get_value(char* ptr) {
    char buf[BUFSIZ] = {0};
    
    while (*(ptr++) && *(ptr - 1) != '=')
        ;

    for (int i = 0; i < BUFSIZ - 1 && *ptr && *ptr != '\n'; ++i)
        buf[i] = *(ptr++);
    
    return string_dup(buf);
}


struct Entry* entry_create(enum EntryType type) {
    struct Entry* new_entry;
    
    new_entry       = malloc(sizeof *new_entry);
    new_entry->type = type;
    new_entry->next = NULL;
    new_entry->prev = NULL;

    new_entry->text             = malloc(sizeof *new_entry->text);
    new_entry->text->str        = NULL;
    new_entry->text->fgcolor    = COLOR_DEFAULT;
    new_entry->text->bgcolor    = COLOR_DEFAULT;

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
        free(data);
    }

    if (entry->text->str)
        free(entry->text->str);

    free(entry->text);
    free(entry);
}


char* parse_color(char* color, int fg) {
    if (color == NULL)
        return NULL;
        
    if (strcmp(color, "black")   == 0)  return fg ? COLOR_FG_BLACK   : COLOR_BG_BLACK;
    if (strcmp(color, "blue")    == 0)  return fg ? COLOR_FG_BLUE    : COLOR_BG_BLUE;
    if (strcmp(color, "cyan")    == 0)  return fg ? COLOR_FG_CYAN    : COLOR_BG_CYAN;
    if (strcmp(color, "green")   == 0)  return fg ? COLOR_FG_GREEN   : COLOR_BG_GREEN;
    if (strcmp(color, "magenta") == 0)  return fg ? COLOR_FG_MAGENTA : COLOR_BG_MAGENTA;
    if (strcmp(color, "red")     == 0)  return fg ? COLOR_FG_RED     : COLOR_BG_RED;
    if (strcmp(color, "white")   == 0)  return fg ? COLOR_FG_WHITE   : COLOR_BG_WHITE;
    if (strcmp(color, "yellow")  == 0)  return fg ? COLOR_FG_YELLOW  : COLOR_BG_YELLOW;

    return NULL;
}


struct Entry* parse_entry_reg(char* ptr) {
    struct Entry* entry;
    struct EntryData_Reg* entry_data;

    entry = entry_create(ET_REG);
    entry_data = malloc(sizeof *entry_data);
    
    entry_data->exec = NULL;
    entry_data->wait = 0;
    entry_data->colormode = CM_NONE;


    while ( (ptr = next_line(ptr)) ) {
        char* val_ptr = NULL;

        if (string_startswith(ptr, "["))
            break;

        if (string_startswith(ptr, "exec=") && entry_data->exec == NULL)
            entry_data->exec = get_value(ptr);

        if (string_startswith(ptr, "wait=") && entry_data->wait == 0)
            entry_data->wait = strcmp((val_ptr = get_value(ptr)), "true") == 0;

        if (string_startswith(ptr, "colormode=") && entry_data->colormode == CM_NONE) {
            val_ptr = get_value(ptr);

            if (strcmp(val_ptr, "selected") == 0) 
                entry_data->colormode = CM_SELECTED;
        }
        
        if (val_ptr)
            free(val_ptr);
    }

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
            entry_curr = entry_create(ET_HEADER);
        
        if (entry_curr) {
            while ( (ptr = next_line(ptr)) ) {
                char* val_ptr = NULL;
                
                if (string_startswith(ptr, "["))
                    break;

                if (string_startswith(ptr, "str="))
                    entry_curr->text->str = get_value(ptr);

                if (string_startswith(ptr, "fgcolor="))
                    entry_curr->text->fgcolor = parse_color((val_ptr = get_value(ptr)), 1);

                if (string_startswith(ptr, "bgcolor="))
                    entry_curr->text->bgcolor = parse_color(val_ptr = get_value(ptr), 0);

                if (val_ptr)
                    free(val_ptr);
            }

            if (entry_head == NULL) {
                entry_head = entry_curr;
            } else {
                entry_append(entry_head, entry_curr);
            }

        } else {
            ptr = next_line(ptr);
        }

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
    printf(vis ? "\033[?25h" : "\033[?25l");
}


void entry_print(struct Entry* entry, int selected) {
    int allow_color = 1;
    
    if (entry->type == ET_REG) {
        struct EntryData_Reg* data = entry->data;
     
        if (data->colormode == CM_SELECTED && !selected)
            allow_color = 0;
    }

    printf("%s%s%s%s%s%s       ",   entry->type == ET_HEADER ? "\033[4m" : "",
                                    allow_color ? entry->text->fgcolor : "", 
                                    allow_color ? entry->text->bgcolor : "", 
                                    entry->text->str,
                                    selected ? " (*)" : "",
                                    COLOR_RESET);
}


void draw(struct Entry* head) {
    struct Entry* cursor = head;
    int x, y;

    x = y = 0;    
    while (cursor) {
        cursor_pos(x, ++y);
        
        if (cursor->type == ET_HEADER) {
            entry_print(cursor, 0);
        }
        
        if (cursor->type == ET_REG)
            entry_print(cursor, (cursor == g_selected));
        
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


int32_t getch(void) {
    char buf[4] = {0};
    return (read(0, buf, 4) != -1) ? *(int32_t*) buf : -1;
}


/* entry_execute is not the correct name for this, since it does not 
   actually do anything to the entry. */
void execute_entry(struct Entry* entry) {
    struct EntryData_Reg* data = entry->data;

    /* Restore so the application we run looks as it should */
    restore_terminal_mode();
    cursor_visible(1);

    system("clear");
    system(data->exec);
    cursor_visible(0);

    if (data->wait) {
        set_terminal_mode();
        printf("Press any key to continue...\n");
        getch();
    } else {
        set_terminal_mode();
    }

    system("clear");
}

int main(int argc, char** argv) {
    struct Entry* head;

    if (argc > 1)
        head = parse_entries(argv[1]);
    else
        error_exit("No entries file provided");

    select_entry(head);
    set_terminal_mode();
    cursor_visible(0);
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
                execute_entry(g_selected);
                break;

            case KEY_Q:
            case KEY_CTRLC:
                goto cleanup;

            case -1:
                error_exit("Failed to read stdin");
                
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
