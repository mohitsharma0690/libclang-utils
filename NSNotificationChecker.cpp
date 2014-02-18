/*
 * NSNotificationChecker.cpp
 *
 * Created By Mohit Sharma
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<Block.h>

#include "clang-c/Index.h"
#include "clang-c/CXString.h"

#define DEBUG 0

/*
 * Every implementation in Objective-C that does an
 * [NSNotificationCenter addObserver:selector:name:object] should also do a
 * [NSNotificationCenter rmeoveObserver:self]
 *
 */

/*
 * create a map of all the classes and their cursors that have the addObserver:
 * call go through all of these cursors and find a dealloc method that removes
 * them from * NSNotificationCenter
 *
 */
using namespace std;

const char *addObserverSelector = "addObserver:selector:name:object:";
const char *removeObserverSelector = "removeObserver:";

/*
 * Print debug information for each cursor.
 * TODO: Make it more generic for all types of cursors.
 */
void printDataForCursor(CXCursor cursor, const char *prefix) {
#if DEBUG
    CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);
    CXString filename;
    unsigned line;
    unsigned column;
    clang_getPresumedLocation(cursorLocation, &filename, &line, &column);
    CXString name = clang_getCursorDisplayName(cursor);
    const char *filenameCString = clang_getCString(filename);
    /*
       Hack to exclude system headers.
       There is a better API method `isInSystemHeader` that would do this in a
       correct way but sadly that doesn't ship with my libclang/clang and I was
       lazy to download the latest one
       */
    if (strstr(filenameCString, "/System") == NULL &&
                strstr(filenameCString, "/usr") == NULL) {
        printf("%s:%s:%u:%u: %s\n",
                prefix, filenameCString, line, column, clang_getCString(name));
    }
    clang_disposeString(filename);
    clang_disposeString(name);
#endif
}

void checkForObserverCount(char *fileName, int addObserverCount, int removeObserverCount) {
    printf("=========================================\n");
    printf("file: %s addObserverCount: %d removeObserverCount: %d\n",
            fileName, addObserverCount, removeObserverCount);
    if (addObserverCount > 0 && removeObserverCount == 0) {
        printf("ObjC Implementation %s doesn't have balanced observerCount\n", fileName);
    }
    printf("=========================================\n\n");
}

void printASTDumpWithPrefix(const char *prefix, CXCursor cursor, const char *fileName) {
    CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);
    CXFile file;
    clang_getExpansionLocation(cursorLocation, &file, NULL, NULL, NULL);
    CXString currentFileName = clang_getFileName(file);
    const char *currentFileNameCStr = clang_getCString(currentFileName);
    if (strcmp(currentFileNameCStr, fileName) == 0) {
        CXString spelling = clang_getCursorSpelling(cursor);
        printf("%s %s\n", prefix, clang_getCString(spelling));
        clang_disposeString(spelling);
    }
    clang_disposeString(currentFileName);
}

void dumpASTForClassesAndFunctions(CXCursor cursor, const char *fileName) {
    CXCursorVisitorBlock dumpASTBlock = ^CXChildVisitResult(CXCursor cursor, CXCursor parent) {
        if (clang_getCursorKind(cursor) == CXCursor_ObjCInterfaceDecl) {
            printASTDumpWithPrefix("|-", cursor, fileName);
            return CXChildVisit_Recurse;
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCPropertyDecl) {
            printASTDumpWithPrefix("|---", cursor, fileName);
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCInstanceMethodDecl) {
            printASTDumpWithPrefix("|---", cursor, fileName);
        }
        return CXChildVisit_Continue;
    };
    clang_visitChildrenWithBlock(cursor, dumpASTBlock);
}

int main() {
    char fileName[] = "Hello.m";
    CXIndex index = clang_createIndex(1, 0);
    const char *args[] = {
        "-I/usr/include",
        "-I."
    };
    int numArgs = sizeof(args) / sizeof(*args);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, fileName, args, numArgs, NULL, 0, CXTranslationUnit_None);
    printf("=========================================\n");
    printf("Translation Unit Loaded: %s\n", clang_getCString(clang_getTranslationUnitSpelling(tu)));
    printf("=========================================\n\n");

#if DEBUG
    dumpASTForClassesAndFunctions(clang_getTranslationUnitCursor(tu), fileName);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
#endif

    // no file name would be more than 1024 chars
    const int maxFilename = 1024;
    __block char *implFileCurrentlyBeingVisited = (char *)malloc(sizeof(char) * maxFilename);
    __block int addObserverCount = 0;
    __block int removeObserverCount = 0;
    CXCursorVisitorBlock block = ^(CXCursor cursor, CXCursor parent) {
        if (clang_getCursorKind(cursor) == CXCursor_ObjCMessageExpr) {
            CXString messageName = clang_getCursorSpelling(cursor);
            const char *messageNameCString = clang_getCString(messageName);
            if (strcmp(messageNameCString, addObserverSelector) == 0) {
                printDataForCursor(parent, "parentCursor");
                printDataForCursor(cursor, "messageExprCursor");
                if (!addObserverCount) {
                    addObserverCount = 1;
                }
            } else if (strcmp(messageNameCString, removeObserverSelector) == 0) {
                printDataForCursor(cursor, "messageExprCursor");
                if (!removeObserverCount) {
                    removeObserverCount = 1;
                }
            }
            clang_disposeString(messageName);
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCImplementationDecl) {
            // new file declaration check for previous file

            if (addObserverCount != 0 || removeObserverCount != 0) {
                checkForObserverCount(implFileCurrentlyBeingVisited, addObserverCount, removeObserverCount);
            }
            // reset counts
            addObserverCount = removeObserverCount = 0;

            printDataForCursor(cursor, "Evaluating Impl");
            CXString name = clang_getCursorSpelling(cursor);
            memset(implFileCurrentlyBeingVisited, 0, maxFilename);
            const char *nameCStr = clang_getCString(name);
            const int nameLen = strlen(nameCStr);
            for (int i = 0; i < nameLen; i++) {
                implFileCurrentlyBeingVisited[i] = nameCStr[i];
            }
            clang_disposeString(name);
        }
        return CXChildVisit_Recurse;
    };
    clang_visitChildrenWithBlock(clang_getTranslationUnitCursor(tu), block);
    checkForObserverCount(implFileCurrentlyBeingVisited, addObserverCount, removeObserverCount);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    free(implFileCurrentlyBeingVisited);
    Block_release(block);
    return 0;
}

