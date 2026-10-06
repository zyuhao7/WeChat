#pragma once

class QObject;

// One factory per test file; tests_main runs them all in a single binary. Suites that
// depend on a signed-in UserMgr run after the ones that check the logged-out state.
QObject *createUserMgrTest();
QObject *createUserListLoadingTest();
QObject *createChatDialogTest();
QObject *createChatDialogTextMsgTest();
