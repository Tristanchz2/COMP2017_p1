#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


struct sound_seg {
    //TODO
    uint32_t size;
    int16_t *data;
};

//stuct for header according to the assumption ASM0.1
struct HEADER {
    //RIFF
    char RIFF_ckID[4]; //RIFF
    uint32_t RIFF_cksize; //chunk size: size - 8
    char WAVEID[4]; //WAVE

    //mft
    char fmt_ckID[4]; //fmt
    uint32_t fmt_cksize; //chunk size: 16 (since we use PCM format)
    uint16_t wFormatTag; //format code: 1 (PCM)
    uint16_t nChannels; //Number of interleaved channels: 1 (mono)
    uint32_t nSamplesPerSec; //sampling rate：8k
    uint32_t nAvgBytesPerSec; //data rate: 8000 * 1 * 16 / 8
    uint16_t nBlockAlign; //data block size: 1 * 16 / 8
    uint16_t wBitsPerSample; //bits per sample: 16
};

// Load a WAV file into buffer
void wav_load(const char* filename, int16_t* dest){
    FILE *file = fopen(filename, "rb");

    //find the chunk of sample data
    char ckID[4];
    uint32_t cksize;

    
    while (fread(ckID, 1, 4, file) == 4){
        if (strncmp(ckID, "RIFF", 4) == 0){
            fseek(file, 8, SEEK_CUR);
            continue;
        }

        fread(&cksize, sizeof(uint32_t), 1, file);
        if (strncmp(ckID, "data", 4) == 0){
            break;
        }
        fseek(file, cksize, SEEK_CUR);
    }

    //read the data into dest
    fread(dest, sizeof(int16_t), cksize / sizeof(int16_t), file);
    fclose(file);
    return;
}

// Create/write a WAV file from buffer
void wav_save(const char* fname, int16_t* src, size_t len){
    struct HEADER header = {
        {'R', 'I', 'F', 'F'},
        32,
        {'W', 'A', 'V', 'E'},
        {'f', 'm', 't', ' '},
        16,
        1,
        1,
        8000,
        16000,
        2,
        16
    };
    
    FILE* file = fopen(fname, "wb");

    uint32_t dataSize = len * sizeof(int16_t);
    //calculate the whole szie 
    header.RIFF_cksize = 4 + 8 + 16 + 8 + dataSize - 8;

    fwrite(&header, sizeof(header), 1, file);

    fwrite("data", 4, 1, file);
    fwrite(&dataSize, 4, 1, file);

    fwrite(src, sizeof(int16_t), len, file);

    fclose(file);

    //free the memory
    // free(src);
    // src = NULL;
    return;
}

// Initialize a new sound_seg object
struct sound_seg* tr_init() {
    //allocate memory in heap
    struct sound_seg* temp = (struct sound_seg*) malloc(sizeof(struct sound_seg));

    //initialize the structure
    temp->size = 0;
    temp->data = NULL;

    return temp;
}

// Destroy a sound_seg object and free all allocated memory
void tr_destroy(struct sound_seg* obj) {
    //free the data pointer
    free(obj->data);
    obj->data = NULL;

    //free the struct itself
    free(obj);
    return;
}

// Return the length of the segment
size_t tr_length(struct sound_seg* seg) {
    return seg->size;
}

// Read len elements from position pos into dest
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len) {
    //memcpy function is in the <string.h> head file
    memcpy(dest, (track->data) + pos, len * sizeof(int16_t));
    return;
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len) {
    if (track->data == NULL){
        track->data = malloc(sizeof(int16_t) * (pos + len));
        track->size = pos + len;
    }else if (track->size < pos + len){
        track->data = realloc(track->data, sizeof(int16_t) * (pos + len));
        track->size = pos + len;
    }
    
    memcpy((track->data) + pos, src, len * sizeof(int16_t));
    return;
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    uint32_t front_ptr = pos;
    uint32_t rear_ptr = pos + len;
    while(rear_ptr < track->size){
        track->data[front_ptr++] = track->data[rear_ptr++];
    }
    
    track->size -= len;
   
    return true;
}

// Fcuntion to compute the cross correlation. Answer could be a large unmber so we use the largest int number we have.
int64_t compute_cross_correlation(const int16_t *input, const int16_t *ad, int len) {
    int64_t result = 0;
    for (int i = 0; i < len; i++) {
        result += (int64_t) (input[i] * ad[i]);
    }
    return result;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad){
    // initialize the dynamic array
    char *result = malloc(1);
    *result = '\0'; // operation below assume there is '\0' at the end
    int result_len = 1;
    
    // calculate the auto correlation
    int64_t auto_cor = compute_cross_correlation(ad->data, ad->data, ad->size);
    if (auto_cor == 0){
        return result;
    }

    // loop to check whole target array
    for (int i = 0; i <= target->size - ad->size; ){
        int64_t cor = compute_cross_correlation(target->data + i, ad->data, ad->size);
        double similarity = (double) cor / (double)auto_cor;
        if (similarity >= 0.95){
            // reallocate the result and update it 
            int extra_len = snprintf(NULL, 0, "%d,%d", i, i + ad->size - 1) + 1; // since snprintf always add '\0' at the end
            
            if (result_len > 1){
                extra_len++; // for '\n'
            }
            
            result = realloc(result, result_len + extra_len);

            if (result_len > 1) {
                strcat(result, "\n");
            }
            snprintf(result + strlen(result), extra_len, "%d,%d", i, i + ad->size - 1);// we need to overwrite the '\0'

            result_len += extra_len - 1; // substract the '\0' length since we overwrite it
            i += ad->size; // skip this segement, avoiding duplicated answer
        }else{
            i++;
        }
    }

    // free the dynamic array
    // char return_arr[result_len];
    // strcpy(return_arr, result);
    // free(result);
    return result;
}


// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {
    return;
}
