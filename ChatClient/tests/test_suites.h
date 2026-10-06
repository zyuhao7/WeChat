#pragma once

class QObject;

// One factory per test file; tests_main runs them all in a single binary.
QObject *createUserListLoadingTest();
QObject *createChatDialogTest();
