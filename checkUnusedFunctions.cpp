#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>

#include "clang-c/Index.h"
#include "clang-c/CXString.h"
#include "clang-c/CXCompilationDatabase.h"


/*
 * create a map of all the classes and their cursors that have the addObserver: call
 * go through all of these cursors and find a dealloc method that removes them from nsnotificiation center
 *
 */
using namespace std;

const char *addObserverSelector = "addObserver:selector:name:object:";
const char *removeObserverSelector = "removeObserver:";

void printDataForCursor(CXCursor cursor, const char *prefix) {
    CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);
    CXString filename;
    unsigned line;
    unsigned column; 
    clang_getPresumedLocation(cursorLocation, &filename, &line, &column);
    CXString name = clang_getCursorDisplayName(cursor);
    const char *filenameCString = clang_getCString(filename);
    /*
       Hack to exclude system headers.
       There is a better API method `isInSystemHeader` that would do this in a correct way
       but sadly that doesn't ship with my libclang and I was lazy to download the latest one
       */
    if (strstr(filenameCString, "/System") == NULL && strstr(filenameCString, "/usr") == NULL) {
        printf("%s:%s:%u:%u: %s\n", 
                prefix, filenameCString, line, column, clang_getCString(name));
    }
    clang_disposeString(filename);
    clang_disposeString(name);
}

void checkForObserverCount(char *fileName, int addObserverCount, int removeObserverCount) {
    printf("file: %s addObserverCount: %d removeObserverCount: %d\n", 
            fileName, addObserverCount, removeObserverCount);
    if (addObserverCount > 0 && removeObserverCount == 0) {
        printf("ObjC Implementation %s doesn't have balanced observerCount\n", fileName);
    }
}

CXCursorVisitorBlock getObjCImplementationCursorAddedToNotifications() {
    __block char *implFileCurrentlyBeingVisited;
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
            } else if (strcmp(messageNameCString, removeObserverSelector)) {
                if (!removeObserverCount) {
                    removeObserverCount = 1;
                }
            }
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCImplementationDecl) {
            // new file declaration check for previous file
            
//            if (addObserverCount || removeObserverCount) {
//                checkForObserverCount(implFileCurrentlyBeingVisited, addObserverCount, removeObserverCount); 
//                addObserverCount = removeObserverCount = 0;
//            }

            printDataForCursor(cursor, "Evaluating Impl: ");
            CXString name = clang_getCursorSpelling(cursor);
            //strcpy(implFileCurrentlyBeingVisited, clang_getCString(name));
            clang_disposeString(name);
        }
        return CXChildVisit_Recurse;
    };
    return block;
}

int main() {
    char fileName[] = "Hello.m";
    CXIndex index = clang_createIndex(0, 0); 
    const char *args[] = {
        "-I/usr/include",
        "-I."
    };
    int numArgs = sizeof(args) / sizeof(*args);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, fileName, args, numArgs, NULL, 0, CXTranslationUnit_None);
    printf("=========================================\n");
    printf("Translation Unit Loaded: %s\n", clang_getCString(clang_getTranslationUnitSpelling(tu)));
    printf("=========================================\n\n");

    /* 
     * Use less block
    CXCursorVisitorBlock getAllMessagesBlock = ^(CXCursor cursor, CXCursor parent) {
        if (clang_getCursorKind(cursor) == CXCursor_ObjCMessageExpr) {
            CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);
            CXString filename;
            unsigned line;
            unsigned column; 
            clang_getPresumedLocation(cursorLocation, &filename, &line, &column);
            CXString name = clang_getCursorSpelling(cursor);
            const char *filenameCString = clang_getCString(filename);
            * 
               Hack to exclude system headers.
               There is a better API method `isInSystemHeader` that would do this in a correct way
               but sadly that doesn't ship with my libclang and I was lazy to download the latest one
               *
            if (strstr(filenameCString, "/System") == NULL && strstr(filenameCString, "/usr") == NULL) {
                printf("parent: %d\n", parent.kind);
                printf("Usage: %s:%u:%u: %s\n", filenameCString, line, column, clang_getCString(name));
            }
            clang_disposeString(filename);
            clang_disposeString(name);
        } else if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {
            //CXString functionDeclaration = clang_getCursorSpelling(cursor);
            //printf("found FunctionDeclaration: %s\n", clang_getCString(functionDeclaration));
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCInstanceMethodDecl) {
            if (parent.kind == CXCursor_ObjCImplementationDecl) {
                CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);
                CXString filename;
                unsigned line;
                unsigned column; 
                clang_getPresumedLocation(cursorLocation, &filename, &line, &column);
                CXString name = clang_getCursorSpelling(cursor);
                const char *filenameCString = clang_getCString(filename);
                *
                   Hack to exclude system headers.
                   There is a better API method `isInSystemHeader` that would do this in a correct way
                   but sadly that doesn't ship with my libclang and I was lazy to download the latest one
                   *
                if (strstr(filenameCString, "/System") == NULL && strstr(filenameCString, "/usr") == NULL) {
                    printf( "parent: %d\n", parent.kind);
                    printf( "%s:%u:%u: %s\n", filenameCString, line, column, clang_getCString(name));
                }
                clang_disposeString(filename);
                clang_disposeString(name);
            }
        }
        return CXChildVisit_Recurse;
    };

    */

    __block char *implFileCurrentlyBeingVisited;
    __block int addObserverCount = 0;
    __block int removeObserverCount = 0;
    CXCursorVisitorBlock block = ^(CXCursor cursor, CXCursor parent) {
        if (clang_getCursorKind(cursor) == CXCursor_ObjCMessageExpr) {
            CXString messageName = clang_getCursorSpelling(cursor);
            const char *messageNameCString = clang_getCString(messageName);
            if (strcmp(messageNameCString, addObserverSelector) == 0) {
                //printDataForCursor(parent, "parentCursor");
                //printDataForCursor(cursor, "messageExprCursor");
                if (!addObserverCount) {
                    addObserverCount = 1;
                }
            } else if (strcmp(messageNameCString, removeObserverSelector) == 0) {
                //printDataForCursor(cursor, "messageExprCursor");
                if (!removeObserverCount) {
                    removeObserverCount = 1;
                }
            }
        } else if (clang_getCursorKind(cursor) == CXCursor_ObjCImplementationDecl) {
            // new file declaration check for previous file
            
            if (addObserverCount != 0 || removeObserverCount != 0) {
                checkForObserverCount(implFileCurrentlyBeingVisited, addObserverCount, removeObserverCount); 
            }
            // reset counts
            addObserverCount = removeObserverCount = 0;

            printDataForCursor(cursor, "Evaluating Impl");
            CXString name = clang_getCursorSpelling(cursor);
            strcpy(implFileCurrentlyBeingVisited, clang_getCString(name));
            clang_disposeString(name);
        }
        return CXChildVisit_Recurse;
    };
    clang_visitChildrenWithBlock(clang_getTranslationUnitCursor(tu), block);
    checkForObserverCount(implFileCurrentlyBeingVisited, addObserverCount, removeObserverCount);
    return 0;
}

