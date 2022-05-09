#ifndef PPARSER_H
#define PPARSER_H

//---------------------------------------------
struct path_root
{
    int drive_no;  // indicates the media of storage, which in our case is 0
    struct path_part* first_part;
};


//------------------------------------------------
struct path_part          // this is basically a linked list object
{
    const char* current_part;
    struct path_part* next_part;
};

// below function takes the path, divides into parts and assign the individual parts 
// to a linked list, and sends the head of linked list back.
struct path_root* pathparser_parse(const char* path, const char* current_directory_path);
void pathparser_free(struct path_root* root);



#endif 