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
    int booksBufferSize;
    int bookNameCounter;
    sem_t fullCount;
    sem_t emptyCount;
    int isFinished;
    pthread_mutex_t isFinishedMutex;
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

void *startPackaging(void *ptr);

int availablePublisherTypeCount();

int publisherTypeSize, publisherSize, packagerSize, bookPerPublisher, booksPerPackage, initBufferSize;

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

    publisherTypes = calloc(publisherTypeSize, sizeof(publisherType));
    publishers = calloc(publisherSize * publisherTypeSize, sizeof(publisher));
    packagers = calloc(packagerSize, sizeof(packager));

    int i;
    void *status;
    for (i = 0; i < publisherTypeSize; i++) { // Creating publisher types.
        publisherTypePtr newPublisherType = calloc(1, sizeof(publisherType));
        newPublisherType->books = calloc(initBufferSize, sizeof(book));
        newPublisherType->booksBufferSize = initBufferSize;
        newPublisherType->bookNameCounter = 0;
        newPublisherType->isFinished = 0;
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

    /*for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) {
            pthread_join(publishers[i* publisherSize + j].publisherId, &status);
        }
    }*/

    int k;
    for (k = 0; k < packagerSize; k++) { // Creating packagers
        packagerPtr newPackager = calloc(1, sizeof(packager));
        newPackager->books = calloc(booksPerPackage, sizeof(book));
        newPackager->bookCount = 0;
        packagers[k] = *newPackager;
        pthread_create(&(newPackager->packagerId), NULL, startPackaging, (void *) newPackager->packagerId);
    }

    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) {
            pthread_join(publishers[i * publisherSize + j].publisherId, &status);
        }
    }

    for (i = 0; i < publisherTypeSize; i++) {
        pthread_join(packagers[i].packagerId, &status);
    }


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

void *startPackaging(void *ptr) {
    int id = (int) ptr;
    int available = availablePublisherTypeCount();

    while (available) {
        int randPublisherTypeIndex = rand() % publisherTypeSize;

        while (publisherTypes[randPublisherTypeIndex].isFinished != 0) {
            randPublisherTypeIndex = rand() % publisherTypeSize;
        }

        int val = -1;
        sem_getvalue(&(publisherTypes[randPublisherTypeIndex].fullCount), &val);

        packBook(id, randPublisherTypeIndex);

        val = -1;
        sem_getvalue(&(publisherTypes[randPublisherTypeIndex].fullCount), &val);

        if (!val && publisherTypes[randPublisherTypeIndex].bookNameCounter == publisherSize * bookPerPublisher) {
            pthread_mutex_lock(&(publisherTypes[randPublisherTypeIndex].isFinishedMutex));
            publisherTypes[randPublisherTypeIndex].isFinished = 1;
            pthread_mutex_unlock(&(publisherTypes[randPublisherTypeIndex].isFinishedMutex));
        }


        available = availablePublisherTypeCount();
    }

}

int availablePublisherTypeCount() {
    int i, count = 0;
    for (i = 0; i < publisherTypeSize; i++) {
        pthread_mutex_lock(&(publisherTypes[i].isFinishedMutex));
        if (publisherTypes[i].isFinished == 0)
            count++;
        pthread_mutex_unlock(&(publisherTypes[i].isFinishedMutex));
    }

    return count;
}

void packBook(int packagerIndex, int publisherTypeIndex) {
    book selectedBook; // might use just names
    int j;

    sem_wait(&(publisherTypes[publisherTypeIndex].fullCount));
    pthread_mutex_lock(&(publisherTypes[publisherTypeIndex].booksBufferMutex));

    for (j = publisherTypes[publisherTypeIndex].booksBufferSize; j >= 0; j--) { // Remove from publisher buffer
        if (publisherTypes[publisherTypeIndex].books[j].name != NULL) { // find last book
            selectedBook = publisherTypes[publisherTypeIndex].books[j];
            publisherTypes[publisherTypeIndex].books[j].name = NULL; // remove book
            //printf("%s removed from publisherType %d\n", selectedBook.name, publisherTypeIndex);
            break;
        }
    }

    pthread_mutex_unlock(&(publisherTypes[publisherTypeIndex].booksBufferMutex));
    sem_post(&(publisherTypes[publisherTypeIndex].emptyCount));

    int i;
    for (i = 0; i < booksPerPackage; i++) {
        if (packagers[packagerIndex].books[i].name == NULL &&
            packagers[packagerIndex].bookCount == booksPerPackage - 1) { // if package is not full yet
            packagers[packagerIndex].books[i].name = selectedBook.name;
            packagers[packagerIndex].bookCount++;
            printf("Packager %d  \t%s Into the package.\n", packagerIndex+1, selectedBook.name);

            printf("Finished preparing one package of packager %d. Package contains:\n", packagerIndex+1);
            int k;
            for(k = 0; k < booksPerPackage; k++) { // empty the package
                printf("%s, ", packagers[packagerIndex].books[k].name); // book name
                packagers[packagerIndex].books[k].name = NULL; // remove
            }
            packagers[packagerIndex].bookCount = 0;
            printf("\n");
            break;
        }

        if(packagers[packagerIndex].books[i].name == NULL) { // if package is not full yet
            packagers[packagerIndex].books[i].name = selectedBook.name;
            packagers[packagerIndex].bookCount++;
            printf("Packager %d  \t%s Into the package.\n", packagerIndex+1, selectedBook.name);
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

    printf("Publisher %d of type %d \tFinished publishing %d books. Exiting the system.\n",
           newBookInfoPtr->publisherId + 1, newBookInfoPtr->publisherTypeId + 1, bookPerPublisher);

}

void insertBook(int publisherIndex, int publisherTypeIndex, book newBook) {
    int i;

    pthread_mutex_lock(&(publisherTypes[publisherTypeIndex].booksBufferMutex));

    int val = -1;
    sem_getvalue(&(publisherTypes[publisherTypeIndex].emptyCount), &val);

    if (val == 0) {
        printf("Publisher %d of type %d \tBuffer is full. Resizing the buffer.\n", publisherIndex + 1,
               publisherTypeIndex + 1);
        bookPtr newBuffer = increaseSizeofBuffer(publisherTypeIndex);
        publisherTypes[publisherTypeIndex].books = newBuffer;
        sem_getvalue(&(publisherTypes[publisherTypeIndex].fullCount), &val);
        sem_init(&(publisherTypes[publisherTypeIndex].emptyCount), 0,
                 publisherTypes[publisherTypeIndex].booksBufferSize - val);
    }

    sem_wait(&(publisherTypes[publisherTypeIndex].emptyCount));
    for (i = 0; i < publisherTypes[publisherTypeIndex].booksBufferSize; i++) {

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
    bookPtr newBuffer = calloc(publisherTypes[publisherTypeIndex].booksBufferSize * 2, sizeof(book));

    int i;
    for (i = 0; i < publisherTypes[publisherTypeIndex].booksBufferSize; i++)
        newBuffer[i] = publisherTypes[publisherTypeIndex].books[i];

    publisherTypes[publisherTypeIndex].booksBufferSize = publisherTypes[publisherTypeIndex].booksBufferSize * 2;

    return newBuffer;
}