libclang-utils
==============
Check if an **Objective-C/Objective-C++** file has unbalanced `NSNotification` `addObserver` and `removeObserver`. Since not removing an object added to `NSNotificationCenter` after `dealloc` leads to an `EXC_BAD_ACCESS` it's useful to check for unbalanced add and remove observers. 

This is still a work in progress.
