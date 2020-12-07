// invertedIndex.c ... implements binary search ADT
// Reads data from files in collection.txt & provides a sorted list
// of file names for every word in a given collection of files.

#include <assert.h>
#include <err.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <math.h>
#include "invertedIndex.h"

#define MAX 100000

static int getWordTotal(char *fileArray);
static double wordCounter(FileList fileNode);
static void fileNodePrint(FileList n, FILE *fp);
static InvertedIndexBST BSTreeNew(void);
static InvertedIndexBST newTreeNode(FileList fileNode, char *word, char *filename, double tf);
static InvertedIndexBST InsertTreeNode(InvertedIndexBST root, char *word, char *filename, double tf, double totalWords);
static FileList newFileNode(char *filename, double tf);
static FileList InsertFileNode(FileList fileNode, char *filename, double tf);
static TfIdfList newTfIdfNode(char *filename, double tfIdf);
static TfIdfList insertTfIdfNode(TfIdfList list, char *filename, double tfIdf);
static TfIdfList listCombine(TfIdfList head1, TfIdfList head2);
static TfIdfList duplicateRemove(TfIdfList head);
static TfIdfList descendingSort(TfIdfList head);
static TfIdfList nodeSwap(TfIdfList node1, TfIdfList node2);
static void printInOrder(FILE *fp, InvertedIndexBST tree);
// static TfIdfList ascendingSort(TfIdfList list, TfIdfList sameTfFile);

char *normaliseWord(char *str) {
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        // Removal of white spaces.
        if (str[i] == ' ') str[i] = '\0';
        if (str[i] == '\n') str[i] = '\0';
        // Converting capital letters to lower case letters.
        if (str[i] >= 'A' && str[i] <= 'Z') str[i] = tolower(str[i]);
    }
    // Only remove punctuation if they occur at the end of a word.
    i--;
    switch(str[i]) {
        case('.'): str[i] = '\0';
        case(','): str[i] = '\0';
        case('?'): str[i] = '\0';
        case(';'): str[i] = '\0';
    }

    return str;
}

// References for utilising an array of strings:
// 1. https://stackoverflow.com/questions/35144153/trying-to-read-list-of-words-from-file-and-store-into-an-array-in-c
// 2. https://stackoverflow.com/questions/40390556/how-to-use-fscanf-to-put-strings-into-an-array
// 3. https://stackoverflow.com/questions/1088622/how-do-i-create-an-array-of-strings-in-c
InvertedIndexBST generateInvertedIndex(char *collectionFilename) {
    InvertedIndexBST tree = BSTreeNew();
    int i = 0, j = 0; double numWordsInFile;
    char *arrfileName[MAX]; char *arrwordName[MAX];
    char origfileName[MAX]; char originalWord[MAX];
    char *fileName; char *normedwordName; 

    FILE *fp1 = fopen(collectionFilename, "r");
    // Read in each .txt file into an array that will store file names.
    while (fscanf(fp1, "%s", origfileName) != EOF) {
        arrfileName[i] = malloc(strlen(origfileName) + 1);
        strcpy(arrfileName[i], origfileName);
        fileName = arrfileName[i];
        // Calculating total number of words in the read file.
        numWordsInFile = getWordTotal(origfileName);

        // Opening each .txt file & reading each word into an array to be stored.
        FILE *fp2 = fopen(origfileName, "r");
        while (fscanf(fp2, "%s", originalWord) != EOF) {
            arrwordName[j] = malloc(strlen(originalWord) + 1);
            strcpy(arrwordName[j], originalWord);
            // Normalising the word and storing it into a BST.
            normedwordName = normaliseWord(arrwordName[j]);
            // Take tf as 1 / number of words in a file.
            tree = InsertTreeNode(tree, normedwordName, fileName, 1 / numWordsInFile, numWordsInFile);
            j++;
        }
        fclose(fp2);
        i++;
    }
    fclose(fp1);

    return tree;
}

// Print to invertedIndex.txt.
// Print using infix order: left -> root -> right.
void printInvertedIndex(InvertedIndexBST tree) {
    if (tree == NULL) 
        return;
    FILE *fp = fopen("invertedIndex.txt", "w");
    if (fp == NULL) 
        exit(1);
    else 
        printInOrder(fp, tree);
    fclose(fp); 
}

TfIdfList calculateTfIdf(InvertedIndexBST tree, char *searchWord, int D) {
    TfIdfList list = NULL;
    if (tree == NULL) 
        return list;
    int cmp = strcmp(searchWord, tree->word);
    double tfIdf, frequency;

    // Go to the node containing the SearchWord
    if (cmp == 0) { 
        FileList curr = tree->fileList;
        // Count the number of times the word occurs in the file.
        frequency = wordCounter(tree->fileList);
        while (curr != NULL) {
            tfIdf = curr->tf * (log10(D / frequency));
            list = insertTfIdfNode(list, curr->filename, tfIdf);
            curr = curr->next;
        }
    }
    if (cmp < 0) 
        return calculateTfIdf(tree->left, searchWord, D);
    if (cmp > 0) 
        return calculateTfIdf(tree->right, searchWord, D);

    // Returns the head of an ordered linked list.
    return list;
}

TfIdfList retrieve(InvertedIndexBST tree, char *searchWords[], int D) {
    TfIdfList originalList = NULL;
    TfIdfList appendList = NULL;
    int i = 0; char *keyWord;

    if (tree == NULL) 
        return originalList;
    // Read each individual word from the array searchWords.
    while (searchWords[i] != NULL) {
        keyWord = searchWords[i];
        // Create a tfIdf list for each searchWord scanned.
        originalList = calculateTfIdf(tree, keyWord, D);
        // Concatentate the current list with the previous list.
        originalList = listCombine(originalList, appendList);
        // Update the previous list.
        appendList = originalList;
        i++;
    }

    return originalList;
}

// Constructor and Destructor
// Creates a new empty tree.
static InvertedIndexBST BSTreeNew(void) {
	return NULL;
}

// Make a new tree node containing "word" to be inserted.
static InvertedIndexBST newTreeNode(FileList fileNode, char *word, char *filename, double tf) {
    InvertedIndexBST t = malloc(sizeof(struct InvertedIndexNode));
    if (t == NULL) 
        exit(EXIT_FAILURE);
    t->word = malloc((strlen(word) + 1) * sizeof(char));
    strcpy(t->word, word);
    t->fileList = InsertFileNode(fileNode, filename, tf);
    t->left = t->right = NULL;
    return t;
}

// Make a new file node containing the filename and tf.
static FileList newFileNode(char *filename, double tf) {
    FileList new = malloc(sizeof(struct FileListNode));
	if (new == NULL) 
        exit(EXIT_FAILURE);
    new->filename = malloc((strlen(filename) + 1) * sizeof(char));
	strcpy(new->filename, filename);
    new->tf = tf;
    new->next = NULL;
	return new;
}

// Make a new TfIdf Node.
static TfIdfList newTfIdfNode(char *filename, double tfIdf) {
    TfIdfList new = malloc(sizeof(struct TfIdfNode));
	if (new == NULL) 
        exit(EXIT_FAILURE);
    new->filename = malloc((strlen(filename) + 1) * sizeof(char));
	strcpy(new->filename, filename);
    new->tfIdfSum = tfIdf;
    new->next = NULL;
    return new;
}

// Inserting a fileNode within a tree node. 
static FileList InsertFileNode(FileList fileNode, char *filename, double tf) {
    if (fileNode == NULL)  
        return newFileNode(filename, tf); 

    int cmp = strcmp(filename, fileNode->filename);
    if (cmp == 0) 
        return fileNode;
    // scanned filename comes before filename at the HEAD of the list.
    if (cmp < 0) {
        FileList temp = fileNode;
        fileNode = newFileNode(filename, tf);
        fileNode->next = temp;
    }
    else { // cmp = 0
        FileList prev = NULL;
        FileList curr = fileNode;
        // Add to the middle or end of the list.
        while (curr->next != NULL && strcmp(filename, curr->filename) > 0) { 
            prev = curr;
            curr = curr->next;
            if (strcmp(filename, curr->filename) == 0) 
                return fileNode;
            if (strcmp(filename, curr->filename) < 0) {
                prev->next = newFileNode(filename, tf);
                prev->next->next = curr;
                return fileNode;
            } 
        }
        // Add to the end of the list.
        curr->next = newFileNode(filename, tf);
    }

    return fileNode;
}

// Inserting a tree node. 
static InvertedIndexBST InsertTreeNode(InvertedIndexBST root, char *word, char *filename, double tf, double totalWords) {
    if (root == NULL) 
        return newTreeNode(NULL, word, filename, tf);

    int wordcmp = strcmp(word, root->word);
    if (wordcmp < 0) root->left = InsertTreeNode(root->left, word, filename, tf, totalWords);
    if (wordcmp > 0) root->right = InsertTreeNode(root->right, word, filename, tf, totalWords);
    if (wordcmp == 0) { // Scanned word already exists
        FileList curr = root->fileList;
        while (curr != NULL) {
            // same word in same file
            if (strcmp(filename, curr->filename) == 0) {
                curr->tf += 1 / totalWords;
            }
            curr = curr->next;
        }
        // filename does not exist
        root->fileList = InsertFileNode(root->fileList, filename, tf);
    }
    return root;
}
 
// Finds total number of words in a file.
static int getWordTotal(char *fileArray) {
    int wordCount = 0;
    char word[MAX];
    FILE *fp = fopen(fileArray, "r");
    while (fscanf(fp, "%s", word) != EOF) 
        wordCount++;
    fclose(fp);
    return wordCount;
}

// Find the number of times a searchWord appears in a given file.
static double wordCounter(FileList fileNode) {
    double occurrences = 0;
    // First search for the word in the tree;
    if (fileNode == NULL) 
        return occurrences;
    FileList curr = fileNode;
    while (curr != NULL) {
        occurrences++;
        curr = curr->next;
    }
    return occurrences;
}

// Returns an ordered linked list of tfIdf Nodes.
static TfIdfList insertTfIdfNode(TfIdfList list, char *filename, double tfIdf) {
 // First insertion.
    if (list == NULL) 
        return newTfIdfNode(filename, tfIdf);

    // Sort by descending order.
    // Checks the first node.
    if (tfIdf > list->tfIdfSum) {
        TfIdfList temp = list;
        list = newTfIdfNode(filename, tfIdf);
        list->next = temp;
        return list;
    }
    else {
        if (tfIdf == list->tfIdfSum) {
        // Even with the same tfIdfs, we go next and filenames will get sorted automatically.      
        }
        // Inserting recursively.
        list->next = insertTfIdfNode(list->next, filename, tfIdf);
    }
    return list;
}
// Appends list2 to list1 to form a new list.
static TfIdfList listCombine(TfIdfList head1, TfIdfList head2) {
    TfIdfList curr = head1;
    if (head1 == NULL || head2 == NULL) return head1;

    // Iterate through first list until the last node.
    while (curr->next != NULL) 
        curr = curr->next;
    
    // Attach the last node from list1 to the head of list 2.
    curr->next = head2;
    // Removes duplicates & sums tfIdfs from newly merged list.
    head1 = duplicateRemove(head1);
    // Sort new list in descending order.
    head1 = descendingSort(head1);
    return head1;
}

// Finds duplicates in an appended linked list, sums the tfIdfs and removes the duplicates.
static TfIdfList duplicateRemove(TfIdfList head) {
    TfIdfList curr = head;
    TfIdfList prev;
    TfIdfList dup;

    if (curr == NULL) 
        return head;

    while (curr != NULL && curr->next != NULL) {
        prev = curr;
        // Compare the chosen node with the rest of the nodes in the list.
        while (prev->next != NULL) {
            if (strcmp(curr->filename, prev->next->filename) == 0) {
                // Update tfIdf.
                curr->tfIdfSum += prev->next->tfIdfSum;
                // Removing duplicate.
                dup = prev->next;
                prev->next = prev->next->next;
                free(dup);
            }
            else 
                prev = prev->next;
        }
        curr = curr->next;
    }
    // Return the modified head.
    return head;
}
// Sorts newly merged linked list into descending order for tfIdfs.
// Recursive iplementation of an adapted bubble sort alogirthm. 
// Reference: 
// 1. https://www.prodevelopertutorial.com/perform-bubble-sort-on-singly-linked-list-solution-in-c/
// 2. https://www.geeksforgeeks.org/c-program-bubble-sort-linked-list/
static TfIdfList descendingSort(TfIdfList head) {
    if (head == NULL) 
        return head;
    // Bring smaller tfIdfs down.
    if (head->next != NULL && head->tfIdfSum < head->next->tfIdfSum)
        head = nodeSwap(head, head->next);
    // Sorting is based around the second node.
    head->next = descendingSort(head->next);
    // Pull bigger tfIdfs up.
    if (head->next != NULL && head->tfIdfSum < head->next->tfIdfSum) {
        head = nodeSwap(head, head->next);
        head->next = descendingSort(head->next);
    }
    return head;
}
// Sorts filenames in alphabetical order.
// static TfIdfList ascendingSort(TfIdfList list, TfIdfList sameTfFile) {
//     if (list == NULL)
//         return sameTfFile;

//     int cmp = strcmp(sameTfFile->filename, list->filename); 
//     if (cmp < 0) { // sameTFfilename is before list->filename
//         sameTfFile->next = list;
//         return sameTfFile;
//     }
//     else { // cmp >= 0
//         list->next = ascendingSort(list->next, sameTfFile);
//     }
//     return list;
// }

// Infix order print.
static void printInOrder(FILE *fp, InvertedIndexBST tree) {
    if (tree == NULL) 
        return;
    // Printing left node.
    printInOrder(fp, tree->left);

    // Printing the root.
    fprintf(fp, "%s ", tree->word);
    fileNodePrint(tree->fileList, fp);

    // Printing right node.
    printInOrder(fp, tree->right);
}

// Prints out fileName and tf contained within each word node recursively.
static void fileNodePrint(FileList n, FILE *fp) {
    if (n == NULL) {
        fprintf(fp, "\n");
        return;
    }
    fprintf(fp, "%s (%lf) ", n->filename, n->tf);
    fileNodePrint(n->next, fp);
}

// Swaps the position of nodes.
static TfIdfList nodeSwap(TfIdfList node1, TfIdfList node2) {
    node1->next = node2->next;
    node2->next = node1;
    return node2;
}
