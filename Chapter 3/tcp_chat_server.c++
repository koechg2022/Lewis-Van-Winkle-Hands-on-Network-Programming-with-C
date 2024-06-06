

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    
#else

#endif




bool is_caps(const char c);

bool is_lower(const char c);

bool is_letter(const char c);

bool is_number(const char c);

bool same_char(const char a, const char b, bool ignore_case = true);

bool same_string(const char* first, const char* second, bool ignore_case = true);

bool all_nums(const char* the_string);

unsigned long string_length(const char* the_string);

char to_caps(const char c);

char to_lower(const char c);

void initialize();

void clean_up();


int main(int len, char** args) {


    return 0;
}