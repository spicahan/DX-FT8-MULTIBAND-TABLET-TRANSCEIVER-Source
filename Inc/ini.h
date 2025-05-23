
#ifndef INI_H_
#define INI_H_

// Define a maximum size for strings and the INI structure
#define MAX_SECTION_NAME_LENGTH 32
#define MAX_KEY_LENGTH 32
#define MAX_VALUE_LENGTH 64
#define MAX_SECTIONS 10
#define MAX_KEYS_PER_SECTION 20
#define MAX_LINE_LENGTH 128

// Structure to hold a key-value pair
typedef struct
{
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    bool present;
} ini_entry_t;

// Structure to hold a section
typedef struct
{
    char name[MAX_SECTION_NAME_LENGTH];
    ini_entry_t entries[MAX_KEYS_PER_SECTION];
    unsigned int num_entries;
    bool present;
} ini_section_t;

// Structure to hold the entire INI data
typedef struct
{
    ini_section_t sections[MAX_SECTIONS];
    unsigned int num_sections;
} ini_data_t;

void parse_ini(const char *buffer, size_t buffer_len, ini_data_t *ini_data);
const char *get_ini_value(const ini_data_t *ini_data, const char *section_name, const char *entry_key);
const ini_section_t *get_ini_section(const ini_data_t *ini_data, const char *section_name);
const char *get_ini_value_from_section(const ini_section_t *section, const char *entry_key);

#endif
