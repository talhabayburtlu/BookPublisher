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
    book *books;
    int bookNameCounter;
    int availablePosition;
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
    publishers = calloc(publisherSize, sizeof(publisher));
    packagers = calloc(packagerSize, sizeof(packager));

    int i;
    void *status;
    for (i = 0; i < publisherTypeSize; i++) { // Creating publisher types.
        publisherTypePtr newPublisherType = calloc(1, sizeof(publisherType));
        newPublisherType->books = calloc(initBufferSize, sizeof(book));
        newPublisherType->bookNameCounter = 0;
        publisherTypes[i] = *newPublisherType;
        pthread_create(&(newPublisherType->publisherTypeId), NULL, createPublishers, (void *) (i + 1));
    }

    for (i = 0; i < publisherTypeSize; i++) {
        pthread_join(publisherTypes[i].publisherTypeId, &status);
    }

    pthread_exit(NULL);
}

void *createPublishers(void *ptr) {
    printf("%d publisher\n", (int) ptr);
    int publisherTypeId = (int) ptr;
    int i;
    void *status;

    for (i = 0; i < publisherSize; i++) { // Creating publishers.
        publisherPtr newPublisher = calloc(1, sizeof(publisher));
        newPublisher->publisherType = &publisherTypes[publisherTypeId - 1];
        pthread_create(&(newPublisher->publisherId), NULL, createBooks, (void *) publisherTypeId);
    }
}

void *createBooks(void *ptr) {
    printf("kitap ediyoruz\n");
    int publisherTypeId = (int) ptr;
    int i;
    void *status;

    for (i = 0; i < bookPerPublisher; i++) { // Creating books.
        bookPtr newBook = calloc(1, sizeof(book));
        newBook->name = calloc(32, sizeof(char));
        publisherTypes[publisherTypeId].bookNameCounter++;
        int bookCount = publisherTypes[publisherTypeId].bookNameCounter;
        printf("%d\n", bookCount);
        // strcpy(newBook->name, sprintf("Book%d_%d", publisherTypeId, bookCount));
        strcpy(newBook->name, "testbook");
        publisherTypes[publisherTypeId].books[publisherTypes[publisherTypeId].availablePosition] = *newBook;
        printf("check3\n");
        publisherTypes[publisherTypeId].availablePosition++;
        // TODO: There should be check about buffer(books array) size. It can be handled with availablePosition is equal to the current size;
    }

}