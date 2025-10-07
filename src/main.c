#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define QUERY_SIZE 100
#define MAX_WORDS 100
#define MAX_WORD_LEN 10

static char *query_buffer[QUERY_SIZE];
static int fp = 0;

int split(const char* string,char*words[]);
int table_root(FILE *file,unsigned short cell_offset,char *table);

char* pop()
{
    if(fp>0)
    {
        return query_buffer[--fp];
    }
    else{
        return NULL;
    }
};
void push(const char * argument)
{
    if(fp>=QUERY_SIZE)
        printf("Query buffer is full\n");
    else
        query_buffer[fp++] = argument;
};


unsigned short serial_to_size(unsigned short serial);
unsigned long long read_varint(FILE *database_file);
void parseCell(FILE *file,unsigned short cell_offset);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./your_program.sh <database path> <command>\n");
        return 1;
    }

    const char *database_file_path = argv[1];
    const char *command = argv[2];

    if (strcmp(command, ".dbinfo") == 0) {
        FILE *database_file = fopen(database_file_path, "rb");
        if (!database_file) {
            fprintf(stderr, "Failed to open the database file\n");
            return 1;
        }

        fseek(database_file, 16, SEEK_SET);  // Skip the first 16 bytes of the header
        unsigned char buffer[2];
        fread(buffer, 1, 2, database_file);
        unsigned short page_size = (buffer[1] | (buffer[0] << 8));

        fseek(database_file,103,SEEK_SET);
        unsigned char table_buffer[2];
        fread(table_buffer, 1, 2, database_file);
        unsigned short number_tables = (table_buffer[1] | (table_buffer[0] << 8));


        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        // Uncomment this to pass the first stage
        printf("database page size: %u\n", page_size);
        printf("number of tables: %u\n",number_tables);


        fseek(database_file,108,SEEK_SET);
        unsigned short cell_offsets[number_tables];
        fread(cell_offsets,2,number_tables,database_file);
        unsigned int absolute_offset = 0;

        for(int i=0;i<number_tables;i++){
            cell_offsets[i] = (cell_offsets[i]>>8) | (cell_offsets[i]<< 8);
            // printf("\\x%04X\n",cell_offsets[i]);
        }
        for(int i=0;i<number_tables;i++){
            parseCell(database_file,cell_offsets[i]);
        }
        

        fclose(database_file);

    }else if(strcmp(command, ".tables") == 0){
        FILE *database_file = fopen(database_file_path, "rb");
        if (!database_file) {
            fprintf(stderr, "Failed to open the database file\n");
            return 1;
        }
        fseek(database_file,103,SEEK_SET);
        unsigned char table_buffer[2];
        fread(table_buffer, 1, 2, database_file);
        unsigned short number_tables = (table_buffer[1] | (table_buffer[0] << 8));

        fseek(database_file,108,SEEK_SET);
        unsigned short cell_offsets[number_tables];
        fread(cell_offsets,2,number_tables,database_file);
        unsigned int absolute_offset = 0;

        for(int i=0;i<number_tables;i++){
            cell_offsets[i] = (cell_offsets[i]>>8) | (cell_offsets[i]<< 8);
            // printf("\\x%04X\n",cell_offsets[i]);
        }
        for(int i=0;i<number_tables;i++){
            parseCell(database_file,cell_offsets[i]);
        }

        fclose(database_file);
        
    } else if(strcmp(command, "SELECT COUNT(*) FROM apples") == 0){
        char * words[MAX_WORDS];
        int words_no = split(command,words);
        if(words_no<=0){
            printf("Trouble splitting the words: debug!!\n");
            return 1;
        }
        char* table_name = words[words_no-1];

        FILE *database_file = fopen(database_file_path,"rb");

        if (!database_file) {
            fprintf(stderr, "Failed to open the database file\n");
            return 1;
        }

        //get page size for pages in db
        fseek(database_file, 16, SEEK_SET);
        unsigned char buffer[2];
        fread(buffer, 1, 2, database_file);
        unsigned short page_size = (buffer[1] | (buffer[0] << 8));

        //get number of tables in the db
        fseek(database_file,103,SEEK_SET);
        unsigned char table_buffer[2];
        fread(table_buffer, 1, 2, database_file);
        unsigned short number_tables = (table_buffer[1] | (table_buffer[0] << 8));

        //getting cell offsets
        fseek(database_file,108,SEEK_SET);
        unsigned short cell_offsets[number_tables];
        fread(cell_offsets,2,number_tables,database_file);
        unsigned int absolute_offset = 0;

        for(int i=0;i<number_tables;i++){
            cell_offsets[i] = (cell_offsets[i]>>8) | (cell_offsets[i]<< 8);
          
        }

        //looking for apple in the schema table
        int root_page;

        for(int i=0;i<number_tables;i++){
            if((root_page=table_root(database_file,cell_offsets[i],table_name))>0)
                break;
        }
        if(root_page){

            fseek(database_file,page_size*(root_page-1),SEEK_SET);
            unsigned char page_type;
            fseek(database_file,3,SEEK_CUR);
            unsigned char record_buffer[2];
            fread(record_buffer, 1, 2, database_file);
            unsigned short number_records = (record_buffer[1] | (record_buffer[0] << 8));
            printf("%u\n",number_records);
        
        }
        else{
            printf("No such table found!!\n");
            return -1;
        }

        
        fclose(database_file);



    }
    else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }

    return 0;
}


void parseCell(FILE *file,unsigned short cell_offset){
     fseek(file, cell_offset,SEEK_SET);
     
     unsigned long long cell_size = read_varint(file);
     unsigned long long row_id = read_varint(file);
     unsigned long long header_size = read_varint(file);
     unsigned short columns_size[5];
    for (int i=0;i<5;i++)
    {
            unsigned long long serial_type = read_varint(file);
            columns_size[i] = serial_to_size(serial_type);
    }

    //reset so we can jump to record body
    fseek(file,columns_size[0]+columns_size[1],SEEK_CUR);

    unsigned char table_name[columns_size[2]+1];
    fread(table_name,1,columns_size[2],file);
    table_name[columns_size[2]]='\0';
    printf("%s \n",table_name);



    fseek(file,columns_size[3],SEEK_CUR);
    unsigned char create_stmt[columns_size[4]+1];
    fread(create_stmt,1,columns_size[4],file);
    create_stmt[columns_size[4]]= '\0';
    printf("CREATE: %s\n",create_stmt);

 }

int table_root(FILE *file,unsigned short cell_offset,char *table)
{
    fseek(file, cell_offset,SEEK_SET);

    read_varint(file);
    read_varint(file);
    read_varint(file);

    unsigned short columns_size[5];
    for (int i=0;i<5;i++)
    {
            unsigned long long serial_type = read_varint(file);
            columns_size[i] = serial_to_size(serial_type);
    }
    
    fseek(file,columns_size[0]+columns_size[1],SEEK_CUR);

    unsigned char table_name[columns_size[2]+1];
    fread(table_name,1,columns_size[2],file);
    table_name[columns_size[2]]='\0';

    if(strcmp(table,table_name)==0){
        unsigned int root_page;
        fread(&root_page,1,columns_size[3],file);
        return root_page;
    }else{
        return 0;
    }
    
        

}
 

unsigned long long read_varint(FILE *database_file)
{
    unsigned char bytes[9];
    unsigned long long result = 0;
    int i =0;

    fread(&bytes,1,1,database_file);

    while(bytes[i] & 0x80){
        i++;
        fread(&bytes[i],1,1,database_file);
    }

    for(int j=0;j<=i;j++)
    {
        result = (result << 7 ) | (bytes[j] & 0X7F);
    }
    return result;
}

unsigned short serial_to_size(unsigned short serial){
    long long result;

    if(serial>=0 && serial <=4)
        return serial;
    else if(serial==5)
        return 6;
    else if(serial==6 || serial==7)
        return 8;
    else if(serial==8 || serial==9)
        return 0;
    else if(serial>=12 && (serial%2==0))
        return (serial-12)/2;
    else if(serial>=13 && (serial%2!=0))
        return (serial-13)/2;
    else    
        return -1;
}


int split(const char* string, char* words[])
{
    const char *copy = string; 
    int word_count = 0;
    int char_index = 0;
    

    while (*copy == ' ') copy++;
    
    while (*copy && word_count < MAX_WORD_LEN) {
        
        const char *word_start = copy;
        
        
        while (*copy && *copy != ' ') {
            copy++;
        }
        
        
        int word_len = copy - word_start;
        
        if (word_len > 0) {
            
            words[word_count] = malloc(word_len + 1);
            if (words[word_count] == NULL) {
                return -1;
            }
            
            strncpy(words[word_count], word_start, word_len);
            words[word_count][word_len] = '\0';  
            word_count++;
        }
        
        while (*copy == ' ') copy++;
    }
    
    return word_count;
}
