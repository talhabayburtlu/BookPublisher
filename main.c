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

void *test(void *ptr) { // This function is not making anything, it just needs to be given to the thread creation call.
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
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

        publisherTypes[i] = *newPublisherType; // Adding created publisher type to global array.
        pthread_create(&(newPublisherType->publisherTypeId), NULL, test,
                       (void *) (i + 1)); // Creating publisher type threads.
    }

    for (i = 0; i < publisherTypeSize; i++) { // Waiting for all publisher types to be created.
        pthread_join(publisherTypes[i].publisherTypeId, &status);
    }


    int j;
    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) { // Creating publishers.
            publisherPtr newPublisher = calloc(1, sizeof(publisher));
            newPublisher->publisherType = &publisherTypes[i];
            publishers[i * publisherSize + j] = *newPublisher;

            bookInfoPtr newBookInfoPtr = calloc(1, sizeof(bookInfo));
            newBookInfoPtr->publisherId = j;
            newBookInfoPtr->publisherTypeId = i;

            pthread_create(&(newPublisher->publisherId), NULL, createBooks,
                           (void *) newBookInfoPtr); // Creating publisher threads
        }
    }

    int k;
    for (k = 0; k < packagerSize; k++) { // Creating packagers
        packagerPtr newPackager = calloc(1, sizeof(packager));
        newPackager->books = calloc(booksPerPackage, sizeof(book));
        newPackager->bookCount = 0;

        packagers[k] = *newPackager; // Adding created packager to global array.
        pthread_create(&(newPackager->packagerId), NULL, startPackaging, (void *) k); // Creating packagers threads.
    }

    for (i = 0; i < publisherTypeSize; i++) {
        for (j = 0; j < publisherSize; j++) { // Waiting for all publishers to be created and create books.
            pthread_join(publishers[i * publisherSize + j].publisherId, &status);
        }
    }

    for (i = 0; i < packagerSize; i++) { // Waiting for all  types to be created.
        pthread_join(packagers[i].packagerId, &status);
    }

    pthread_exit(NULL);
}

void *startPackaging(void *ptr) { // Starts packaging for packager thread.
    int id = (int) ptr;

    // Checking available publisher types count for understanding if program needs to be wait for books to be published.
    int available = availablePublisherTypeCount();

    while (available) {
        int randPublisherTypeIndex = rand() % publisherTypeSize; // Taking a random index for publisher type index.

        while (publisherTypes[randPublisherTypeIndex].isFinished !=
               0) { // Checking if this publisher type is finished or not.
            randPublisherTypeIndex = rand() % publisherTypeSize;
        }

        packBook(id, randPublisherTypeIndex); // Packing book for this packager.

        int val = -1;
        sem_getvalue(&(publisherTypes[randPublisherTypeIndex].fullCount), &val);

        // Checking if the publisher type has finished or not by looking total book count and value of
        // fullCount of books buffer.

        // IMPORTANT NOTE: Even if we lock finished mutex, the program is not able to understand multiple types are finished or not.
        // This breaks the logic of available variable. And it makes while loop iterates one more. In one of other
        // semaphore wait function the program gets inside a deadlock.
        if (!val && publisherTypes[randPublisherTypeIndex].bookNameCounter == publisherSize * bookPerPublisher) {
            pthread_mutex_lock(&(publisherTypes[randPublisherTypeIndex].isFinishedMutex));
            publisherTypes[randPublisherTypeIndex].isFinished = 1;
            pthread_mutex_unlock(&(publisherTypes[randPublisherTypeIndex].isFinishedMutex));
        }

        // Checking available publisher types count for understanding if program needs to be wait for books to be published.
        available = availablePublisherTypeCount(); // Che
    }

    if (available == 0) { // finished
        int x;
        int j;
        for (j = 0;
             j < packagerSize; j++) { // Printing each packager information if there are any books inside it's package.
            printf("There are no publishers left in the system. Only %d of %d "
                   "number of books could be packaged.\n", packagers[id].bookCount, booksPerPackage);
            printf("The package contains: \n");
            for (x = 0; x < booksPerPackage; x++) {
                if (packagers[j].books[x].name != NULL) {
                    printf("%s, ", packagers[id].books[x].name); // book name
                    packagers[j].books[x].name = NULL; // remove
                }
            }
        }
    }

    pthread_exit(NULL);
}

int availablePublisherTypeCount() { // Returns the value of how many available publisher types.
    int i, count = 0;

    for (i = 0; i < publisherTypeSize; i++) {
        pthread_mutex_lock(&(publisherTypes[i].isFinishedMutex));
        if (publisherTypes[i].isFinished == 0) {
            count++;
        }
        pthread_mutex_unlock(&(publisherTypes[i].isFinishedMutex));
    }

    return count;
}

void packBook(int packagerIndex, int publisherTypeIndex) { // Packs book for determined packager.
    book selectedBook; // might use just names
    int j;

    int val = -1;
    sem_getvalue(&(publisherTypes[publisherTypeIndex].fullCount), &val);

    sem_wait(&(publisherTypes[publisherTypeIndex].fullCount)); // Waiting here if there are no books inside buffer.
    pthread_mutex_lock(&(publisherTypes[publisherTypeIndex].booksBufferMutex)); // Locks buffer for delete purposes.

    for (j = publisherTypes[publisherTypeIndex].booksBufferSize - 1; j >= 0; j--) { // Remove from publisher buffer
        if (publisherTypes[publisherTypeIndex].books[j].name != NULL) { // find last book
            selectedBook = publisherTypes[publisherTypeIndex].books[j];
            publisherTypes[publisherTypeIndex].books[j].name = NULL; // remove book
            break;
        }
    }

    pthread_mutex_unlock(&(publisherTypes[publisherTypeIndex].booksBufferMutex)); // Unlocks buffer.
    sem_post(&(publisherTypes[publisherTypeIndex].emptyCount)); // Increases the value of empty count.

    int i;
    for (i = 0; i < booksPerPackage; i++) { // Checking if it's last index of package.
        if (packagers[packagerIndex].books[i].name == NULL &&
            packagers[packagerIndex].bookCount == booksPerPackage - 1) { // if package is not full yet
            packagers[packagerIndex].books[i].name = calloc(25, sizeof(char));
            strcpy(packagers[packagerIndex].books[i].name, selectedBook.name);
            packagers[packagerIndex].bookCount++;
            printf("Packager %d  \t%s Into the package.\n", packagerIndex + 1, selectedBook.name);

            printf("Finished preparing one package of packager %d. Package contains:\n", packagerIndex + 1);
            int k;
            for (k = 0; k < booksPerPackage; k++) { // empty the package
                printf("%s, ", packagers[packagerIndex].books[k].name); // book name
                packagers[packagerIndex].books[k].name = NULL; // remove
            }
            printf("\n");
            packagers[packagerIndex].bookCount = 0;
            break;
        }

        if(packagers[packagerIndex].books[i].name == NULL) { // if package is not full yet
            packagers[packagerIndex].books[i].name = calloc(25, sizeof(char));
            strcpy(packagers[packagerIndex].books[i].name, selectedBook.name);
            packagers[packagerIndex].bookCount++;
            printf("Packager %d  \t%s Into the package.\n", packagerIndex + 1, selectedBook.name);
            break;
        }

    }

}

void *createBooks(void *ptr) { // Creates books for given publisher and publisher type.
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

        insertBook(newBookInfoPtr->publisherId, newBookInfoPtr->publisherTypeId,
                   *newBook); // Inserting book to the buffer.
    }

    printf("Publisher %d of type %d \tFinished publishing %d books. Exiting the system.\n",
           newBookInfoPtr->publisherId + 1, newBookInfoPtr->publisherTypeId + 1, bookPerPublisher);

    pthread_exit(NULL);
}

void insertBook(int publisherIndex, int publisherTypeIndex, book newBook) { // Inserts book to the buffer.
    int i;

    pthread_mutex_lock(&(publisherTypes[publisherTypeIndex].booksBufferMutex)); // Locking buffer for insert purposes.

    int val = -1;
    sem_getvalue(&(publisherTypes[publisherTypeIndex].emptyCount),
                 &val); // Checking value of emptyCount for resizing purpose.

    if (val == 0) { // Resizing buffer by doubling size.
        printf("Publisher %d of type %d \tBuffer is full. Resizing the buffer.\n", publisherIndex + 1,
               publisherTypeIndex + 1);
        bookPtr newBuffer = increaseSizeofBuffer(publisherTypeIndex);
        publisherTypes[publisherTypeIndex].books = newBuffer;
        sem_getvalue(&(publisherTypes[publisherTypeIndex].fullCount), &val);
        sem_init(&(publisherTypes[publisherTypeIndex].emptyCount), 0,
                 publisherTypes[publisherTypeIndex].booksBufferSize - val);
    }

    sem_wait(
            &(publisherTypes[publisherTypeIndex].emptyCount)); // Creating a wait function for decreasing empty counter.
    for (i = 0; i < publisherTypes[publisherTypeIndex].booksBufferSize; i++) {

        if (publisherTypes[publisherTypeIndex].books[i].name == NULL) { // Insert operation happens.
            publisherTypes[publisherTypeIndex].books[i] = newBook;
            printf("Publisher %d of type %d \t%s is published and put into the buffer %d\n", publisherIndex + 1,
                   publisherTypeIndex + 1, newBook.name, publisherTypeIndex + 1);
            break;
        }
    }

    sem_post(&(publisherTypes[publisherTypeIndex].fullCount)); // Increasing the value of full count.
    pthread_mutex_unlock(&(publisherTypes[publisherTypeIndex].booksBufferMutex)); // Unlocking buffer.
}

bookPtr
increaseSizeofBuffer(int publisherTypeIndex) { // Creates a new buffer with doubled size and adds old records to it.
    bookPtr newBuffer = calloc(publisherTypes[publisherTypeIndex].booksBufferSize * 2, sizeof(book));

    int i;
    for (i = 0; i < publisherTypes[publisherTypeIndex].booksBufferSize; i++)
        newBuffer[i] = publisherTypes[publisherTypeIndex].books[i];

    publisherTypes[publisherTypeIndex].booksBufferSize = publisherTypes[publisherTypeIndex].booksBufferSize * 2;

    return newBuffer;
}