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
    book *books;
    int bookCount;
} packager, *packagerPtr;

typedef struct {
    pthread_t publisherTypeId;
    pthread_t publisherId;
} bookInfo, *bookInfoPtr;


void *createPublishers(void *ptr);

void *createBooks(void *ptr);

void insertBook(int publisherIndex, int publisherTypeIndex, book newBook);

bookPtr increaseSizeofBuffer(int publisherTypeIndex);

void packBook(int packagerIndex, int publisherTypeIndex);

int publisherTypeSize, publisherSize, packagerSize, bookPerPublisher, booksPerPackage, initBufferSize, currBufferSize;

publisherTypePtr publisherTypes;
publisherPtr publishers;
packagerPtr packagers;

void *test(void *ptr) {
    // printf("Created publisher type with id:%d\n" , (int) ptr);
}

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
        pthread_create(&(newPublisherType->publisherTypeId), NULL, test, (void *) (i + 1));
    }

    for (i = 0; i < publisherTypeSize; i++) {
        pthread_join(publisherTypes[i].publisherTypeId, &status);
    }


    int j;
    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) { // Creating publishers.
            publisherPtr newPublisher = calloc(1, sizeof(publisher));
            newPublisher->publisherType = &publisherTypes[i];
            publishers[i * publisherSize + j] = *newPublisher;
            // printf("Created publisher with id:%d for type id=%d\n" , j+1, i+1);

            bookInfoPtr newBookInfoPtr = calloc(1, sizeof(bookInfo));
            newBookInfoPtr->publisherId = j;
            newBookInfoPtr->publisherTypeId = i;

            pthread_create(&(newPublisher->publisherId), NULL, createBooks, (void *) newBookInfoPtr);
        }
    }


    int k;
    for(k = 0; k < packagerSize; k++) { // Creating packagers
        packagerPtr newPackager = calloc(1, sizeof(packager));
        newPackager->books = calloc(booksPerPackage, sizeof(book));
        newPackager->bookCount = 0;
        packagers[k] = *newPackager;
    }

    // WHILE LOOP İÇİNDE BUFFER BOŞ OLANA KADAR PACKBOOK RANDOM SAYILARLA ÇALIŞSIN


    /*for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) {
            pthread_join(publishers[i* publisherSize + j].publisherId, &status);
        }
    }*/

    pthread_exit(NULL);
}

/*void *createPublishers(void *ptr) {
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
}*/

void packBook(int packagerIndex, int publisherTypeIndex) {
    book selectedBook; // might use just names
    int j;
    for (j = currBufferSize; j >= 0; j--){ // Remove from publisher buffer
        if(publisherTypes[publisherTypeIndex].books[j].name != NULL) { // find last book
            selectedBook = publisherTypes[publisherTypeIndex].books[j];
            publisherTypes[publisherTypeIndex].books->name = NULL; // remove book
            printf("book %s removed from publisherType %d\n", selectedBook.name, publisherTypeIndex);
            break;
        }
    }

    int i;
    for (i = 0; i < booksPerPackage; i++) {
        if(packagers[packagerIndex].books[i].name == NULL && packagers[packagerIndex].bookCount == booksPerPackage -1) { // if package is not full yet
            packagers[packagerIndex].books[i].name = selectedBook.name;
            packagers[packagerIndex].bookCount++;
            printf("book %s inserted to package of packager %d\n", selectedBook.name, packagerIndex);

            printf("Finished preparing one package of packager %d. Package contains:\n", packagerIndex);
            int k;
            for(k = 0; k < booksPerPackage; k++) { // empty the package
                printf("%s, ", packagers[packagerIndex].books[i].name); // book name
                packagers[packagerIndex].books[i].name = NULL; // remove
            }
            packagers[packagerIndex].bookCount = 0;
            printf("\n");
            break;
        }

        if(packagers[packagerIndex].books[i].name == NULL) { // if package is not full yet
            packagers[packagerIndex].books[i].name = selectedBook.name;
            packagers[packagerIndex].bookCount++;
            printf("book %s inserted to package of packager %d\n", selectedBook.name, packagerIndex);
            break;
        }

    }

}

void *createBooks(void *ptr) {
    bookInfoPtr newBookInfoPtr = ((bookInfoPtr) ptr);
    int i;
    void *status;

    for (i = 0; i < bookPerPublisher; i++) { // Creating books.
        bookPtr newBook = calloc(1, sizeof(book));
        newBook->name = calloc(32, sizeof(char));
        publisherTypes[newBookInfoPtr->publisherTypeId - 1].bookNameCounter++;
        int bookCount = publisherTypes[newBookInfoPtr->publisherTypeId - 1].bookNameCounter;
        char *bookname = calloc(32, sizeof(char));
        sprintf(bookname, "Book%d_%d", newBookInfoPtr->publisherTypeId + 1, bookCount);
        strcpy(newBook->name, bookname);

        insertBook(newBookInfoPtr->publisherId, newBookInfoPtr->publisherTypeId, *newBook);
    }

}

void insertBook(int publisherIndex, int publisherTypeIndex, book newBook) {
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
            printf("Publisher %d of type %d \t%s is published and put into the buffer %d\n", publisherIndex + 1,
                   publisherTypeIndex + 1, newBook.name, publisherTypeIndex + 1);
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