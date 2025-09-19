#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
        
    }else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }

    return 0;
}


void parseCell(FILE *file,unsigned short cell_offset){
     fseek(file, cell_offset,SEEK_SET);
     unsigned long cell_size = read_varint(file);
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
    printf("%s ",table_name);

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
