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

#define PROPERTY(key_str, data_type) {.key = key_str, .type = data_type, .val = NULL }
#define COUNT_OF(arr) (sizeof arr / sizeof *arr)


enum ColorMode {
    CM_NONE     = 0,
    CM_SELECTED
};


enum EntryType {
    ET_NONE     = 0,
    ET_REG,
    ET_HEADER
};


enum KeyType {
    KT_NONE = 0,
    KT_STR,
    KT_INT,
    KT_BOOL
};


struct EntryKey {
    char* key;
    char* val;
    int   type;
};


struct Entry {
    struct Entry* next;
    struct Entry* prev;
    struct EntryKey* keys;
    size_t keys_count;
    size_t index;
    char   type;
};


struct EntryKey g_parameter_keys[] = { 
    PROPERTY("str",         KT_STR),
    PROPERTY("fgcolor",     KT_STR),
    PROPERTY("bgcolor",     KT_STR),
    PROPERTY("exec",        KT_STR),
    PROPERTY("colormode",   KT_STR),
    PROPERTY("wait",        KT_STR)
};

struct Entry*  g_selected    = NULL;
size_t         g_entry_count = 0;
struct termios g_termios_original;


void error_exit(char* msg) {
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}


void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    
    if (ptr == NULL)
        error_exit("malloc failed");
    
    return ptr;
}


void* xcalloc(size_t n, size_t size) {
    void* ptr = calloc(n, size);
    
    if (ptr == NULL)
        error_exit("calloc failed");
    
    return ptr;
}


void clear_screen(void) {
    system("clear");
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
        error_exit("failed to open file");

    file_size = get_filesize(file_ptr);
    buf = xcalloc(1, file_size + 1);

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
    char* new_str = xcalloc(1, strlen(str) + 1);

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
    struct Entry* new_entry = xmalloc(sizeof *new_entry);;

    new_entry->type         = type;
    new_entry->next         = NULL;
    new_entry->prev         = NULL;
    new_entry->index        = g_entry_count++;
    new_entry->keys_count   = 0;
    new_entry->keys         = NULL;

    return new_entry;
}


void entry_append(struct Entry* head, struct Entry* node) {
    while (head->next)
        head = head->next;

    head->next = node;
    node->prev = head;
}


void entry_destroy(struct Entry* entry) {
    if (entry->keys) {
        for (size_t i = 0; i < entry->keys_count; ++i)
            free(entry->keys[i].val);

        free(entry->keys);
    }

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

    return "";
}


void entry_copy_keys(struct Entry* entry) {
    int j = 0;
    entry->keys = xmalloc(entry->keys_count * sizeof *entry->keys);  
    
    for (size_t i = 0; i < COUNT_OF(g_parameter_keys); ++i) {
        if (g_parameter_keys[i].val) {
            entry->keys[j++] = g_parameter_keys[i];
            g_parameter_keys[i].val = NULL;
        }
    }
}


struct Entry* parse_entry(char* ptr) {
    struct Entry* entry;
    
    if      (string_startswith(ptr, "[Entry]"))  entry = entry_create(ET_REG);
    else if (string_startswith(ptr, "[Header]")) entry = entry_create(ET_HEADER);
    else return NULL;
    
    while ( (ptr = next_line(ptr)) ) {
        if (string_startswith(ptr, "["))
            break;
            
        for (size_t i = 0; i < COUNT_OF(g_parameter_keys); ++i) {
            if (string_startswith(ptr, g_parameter_keys[i].key)) {
                g_parameter_keys[i].val = get_value(ptr);
                ++entry->keys_count;
            }    
        }
    }

    entry_copy_keys(entry);
    return entry;
}


struct Entry* parse_entries(char* entries_path) {
    struct Entry* entry_head = NULL;
    struct Entry* entry_curr = NULL;

    char* file_content = read_file(entries_path);
    char* ptr          = file_content;
    
    do {
        entry_curr = string_startswith(ptr, "[") ? parse_entry(ptr) : NULL;

        if (entry_curr) {
            if (entry_head)
                entry_append(entry_head, entry_curr);
            else
                entry_head = entry_curr;
        }

    } while ( (ptr = next_line(ptr)) );

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


char* get_key(struct Entry* entry, char* key) {
    for (size_t i = 0; i < entry->keys_count; ++i)
        if (strcmp(key, entry->keys[i].key) == 0)
            return entry->keys[i].val;

    return "";
}


void entry_print(struct Entry* entry, int selected) {
    int allow_color = 1;

    if (entry->type == ET_REG && strcmp(get_key(entry, "colormode"), "selected") == 0)
        allow_color = selected;

    printf("%s%s%s%s%s%s       ",   entry->type == ET_HEADER ? "\033[4m" : "",
                                    allow_color ? parse_color(get_key(entry, "fgcolor"), 1) : "",
                                    allow_color ? parse_color(get_key(entry, "bgcolor"), 0) : "",
                                    get_key(entry, "str"),
                                    selected ? " (*)" : "",
                                    COLOR_RESET);
}


void draw(struct Entry* head) {
    struct Entry* cursor = head;
    struct winsize window;
    int x, y;

    ioctl(0, TIOCGWINSZ, &window);
    clear_screen();

    /* Scrolling */
    for (size_t i = 0; (window.ws_row + i) <= g_selected->index; ++i) {
        if (cursor->next)
            cursor = cursor->next;
    }

    x = y = 0;
    while (cursor) {
        if (++y <= window.ws_row)
            cursor_pos(x, y);
        else
            break;

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
    /* Restore so the application we run looks as it should */
    restore_terminal_mode();
    cursor_visible(1);

    clear_screen();
    system(get_key(entry, "exec"));
    cursor_visible(0);

    if (*get_key(entry, "wait")) {
        set_terminal_mode();
        printf("Press any key to continue...\n");
        getch();
    } else {
        set_terminal_mode();
    }

    clear_screen();
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
    clear_screen();

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
    clear_screen();
    return EXIT_SUCCESS;
}
