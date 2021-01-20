#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

typedef struct {
    char *name;
} book, *bookPtr;

typedef struct {
    pthread_t publisherTypeId;
    pthread_mutex_t booksBufferMutex;
    book *books;
    int bookNameCounter;
    sem_t fullCount;
    sem_t emptyCount;
} publisherType, *publisherTypePtr;

typedef struct {
    pthread_t publisherId;
    publisherTypePtr publisherType;
} publisher, *publisherPtr;

typedef struct {
    pthread_t packagerId;
} packager, *packagerPtr;


void *createPublishers(void *ptr);

void *createBooks(void *ptr);

void insertBook(int publisherTypeIndex, book newBook);

bookPtr increaseSizeofBuffer(int publisherTypeIndex);

int publisherTypeSize, publisherSize, packagerSize, bookPerPublisher, booksPerPackage, initBufferSize, currBufferSize;

publisherTypePtr publisherTypes;
publisherPtr publishers;
packagerPtr packagers;

int main(int argc, char *argv[]) {
    printf("Hello, World!\n");

    publisherTypeSize = atoi(argv[2]), publisherSize = atoi(argv[3]), packagerSize = atoi(argv[4]),
    bookPerPublisher = atoi(argv[6]), booksPerPackage = atoi(argv[8]), initBufferSize = atoi(argv[9]),
    currBufferSize = initBufferSize;

    publisherTypes = calloc(publisherTypeSize, sizeof(publisherType));
    publishers = calloc(publisherSize * publisherTypeSize, sizeof(publisher));
    packagers = calloc(packagerSize, sizeof(packager));

    int i;
    void *status;
    for (i = 0; i < publisherTypeSize; i++) { // Creating publisher types.
        publisherTypePtr newPublisherType = calloc(1, sizeof(publisherType));
        newPublisherType->books = calloc(initBufferSize, sizeof(book));
        newPublisherType->bookNameCounter = 0;
        sem_init(&(newPublisherType->emptyCount), 0, initBufferSize);
        sem_init(&(newPublisherType->fullCount), 0, 0);
        publisherTypes[i] = *newPublisherType;
        pthread_create(&(newPublisherType->publisherTypeId), NULL, createPublishers, (void *) (i + 1));
    }

    for (i = 0; i < publisherTypeSize; i++) {
        pthread_join(publisherTypes[i].publisherTypeId, &status);
    }

    /*int j;
    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) { // Creating publishers.
            publisherPtr newPublisher = calloc(1, sizeof(publisher));
            newPublisher->publisherType = &publisherTypes[i];
            publishers[i* publisherSize + j] = *newPublisher;
            pthread_create(&(newPublisher->publisherId), NULL, createBooks, (void *) i + 1);
        }
    }

    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) {
            pthread_join(publishers[i* publisherSize + j].publisherId, &status);
        }
    }*/

    pthread_exit(NULL);
}

void *createPublishers(void *ptr) {
    // printf("%d publisher\n", (int) ptr);
    int publisherTypeId = (int) ptr;
    int i;
    void *status;

    for (i = 0; i < publisherSize; i++) { // Creating publishers.
        publisherPtr newPublisher = calloc(1, sizeof(publisher));
        newPublisher->publisherType = &publisherTypes[publisherTypeId - 1];
        publishers[(publisherTypeId - 1) * publisherSize + i] = *newPublisher;
        pthread_create(&(newPublisher->publisherId), NULL, createBooks, (void *) publisherTypeId);
    }
}

void *createBooks(void *ptr) {
    // printf("kitap ediyoruz\n");
    int publisherTypeId = (int) ptr;
    int i;
    void *status;

    for (i = 0; i < bookPerPublisher; i++) { // Creating books.
        bookPtr newBook = calloc(1, sizeof(book));
        newBook->name = calloc(32, sizeof(char));
        publisherTypes[publisherTypeId - 1].bookNameCounter++;
        int bookCount = publisherTypes[publisherTypeId - 1].bookNameCounter;
        char *bookname = calloc(32, sizeof(char));
        sprintf(bookname, "Book%d_%d", publisherTypeId, bookCount);
        // printf("Created %s\n" , bookname);
        strcpy(newBook->name, bookname);

        insertBook(publisherTypeId - 1, *newBook);
    }

}

void insertBook(int publisherTypeIndex, book newBook) {
    int i;
    sem_wait(&(publisherTypes[publisherTypeIndex].emptyCount));
    pthread_mutex_lock(&(publisherTypes[publisherTypeIndex].booksBufferMutex));
    for (i = 0; i < currBufferSize; i++) {
        int val = -1;
        sem_getvalue(&(publisherTypes[publisherTypeIndex].emptyCount), &val);

        if (val == 0) {
            bookPtr newBuffer = increaseSizeofBuffer(publisherTypeIndex);
            publisherTypes[publisherTypeIndex].books = newBuffer;
            sem_getvalue(&(publisherTypes[publisherTypeIndex].fullCount), &val);
            sem_init(&(publisherTypes[publisherTypeIndex].emptyCount), 0, currBufferSize - val);
        }

        // TODO: There should be check about buffer(books array) size. It can be handled with availablePosition is equal to the current size;
        if (publisherTypes[publisherTypeIndex].books[i].name == NULL) {
            publisherTypes[publisherTypeIndex].books[i] = newBook;
            printf("inserted %s\n", newBook.name);
            break;
        }
    }

    sem_post(&(publisherTypes[publisherTypeIndex].fullCount));
    pthread_mutex_unlock(&(publisherTypes[publisherTypeIndex].booksBufferMutex));
}

bookPtr increaseSizeofBuffer(int publisherTypeIndex) {
    bookPtr newBuffer = calloc(currBufferSize * 2, sizeof(book));

    int i;
    for (i = 0; i < currBufferSize; i++)
        newBuffer[i] = publisherTypes[publisherTypeIndex].books[i];

    currBufferSize = currBufferSize * 2;
    return newBuffer;
}