#include <QtTest>

#include "filebubble.h"
#include "filetransfermgr.h"
#include "global.h"

// The chunk arithmetic and the name cleaner both decide whether a transfer survives the trip,
// and a mismatch between sender and receiver only shows up as a stalled or corrupt file. The
// transfer manager itself needs a live socket and a writable Downloads dir, so only the pure
// helpers are covered here.
class FileTransferHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void chunkCountRoundsUp()
    {
        QCOMPARE(FileTransferMgr::ChunkCount(0), 0);
        QCOMPARE(FileTransferMgr::ChunkCount(-1), 0);
        QCOMPARE(FileTransferMgr::ChunkCount(1), 1);
        QCOMPARE(FileTransferMgr::ChunkCount(FILE_CHUNK_BYTES), 1);
        QCOMPARE(FileTransferMgr::ChunkCount(FILE_CHUNK_BYTES + 1), 2);
        QCOMPARE(FileTransferMgr::ChunkCount(FILE_CHUNK_BYTES * 3), 3);
        QCOMPARE(FileTransferMgr::ChunkCount(MAX_FILE_BYTES),
                 static_cast<int>(MAX_FILE_BYTES / FILE_CHUNK_BYTES));
    }

    void chunkLengthCoversEveryByteExactlyOnce()
    {
        for (const qint64 size : {qint64(1), qint64(FILE_CHUNK_BYTES), qint64(FILE_CHUNK_BYTES + 1),
                                  qint64(10 * FILE_CHUNK_BYTES + 7), MAX_FILE_BYTES})
        {
            const int total = FileTransferMgr::ChunkCount(size);
            qint64 sum = 0;
            for (int i = 0; i < total; ++i)
            {
                const qint64 len = FileTransferMgr::ChunkLength(size, i);
                // every chunk but the last is full-sized, and none is ever empty
                QVERIFY(len > 0);
                QVERIFY(len <= FILE_CHUNK_BYTES);
                if (i + 1 < total)
                    QCOMPARE(len, qint64(FILE_CHUNK_BYTES));
                sum += len;
            }
            QCOMPARE(sum, size);
        }
    }

    void chunkLengthRejectsOutOfRange()
    {
        const qint64 size = FILE_CHUNK_BYTES * 2 + 5;
        QCOMPARE(FileTransferMgr::ChunkLength(size, -1), qint64(0));
        QCOMPARE(FileTransferMgr::ChunkLength(size, 3), qint64(0));
        QCOMPARE(FileTransferMgr::ChunkLength(size, 2), qint64(5));
        QCOMPARE(FileTransferMgr::ChunkLength(0, 0), qint64(0));
    }

    void sanitizeKeepsTheLeafOnly()
    {
        QCOMPARE(FileTransferMgr::SanitizeFileName("report.pdf"), QString("report.pdf"));
        QCOMPARE(FileTransferMgr::SanitizeFileName("../../etc/passwd"), QString("passwd"));
        QCOMPARE(FileTransferMgr::SanitizeFileName("C:\\Users\\me\\notes.txt"), QString("notes.txt"));
        QCOMPARE(FileTransferMgr::SanitizeFileName("/tmp/a/b/c.tar.gz"), QString("c.tar.gz"));
    }

    void sanitizeRejectsTraversalAndEmpty()
    {
        QVERIFY(FileTransferMgr::SanitizeFileName("..").isEmpty());
        QVERIFY(FileTransferMgr::SanitizeFileName(".").isEmpty());
        QVERIFY(FileTransferMgr::SanitizeFileName("dir/..").isEmpty());
        QVERIFY(FileTransferMgr::SanitizeFileName("").isEmpty());
        QVERIFY(FileTransferMgr::SanitizeFileName("   ").isEmpty());
    }

    void sanitizeReplacesCharactersTheServerRejects()
    {
        QCOMPARE(FileTransferMgr::SanitizeFileName("a:b.txt"), QString("a_b.txt"));
        QCOMPARE(FileTransferMgr::SanitizeFileName("tab\there"), QString("tab_here"));
        // backslashes are separators, not literal name characters
        QCOMPARE(FileTransferMgr::SanitizeFileName("a\\b"), QString("b"));
    }

    void sanitizeTruncatesLongNamesKeepingTheExtension()
    {
        const QString long_name = QString(200, QChar('x')) + ".txt";
        const QString cleaned = FileTransferMgr::SanitizeFileName(long_name);
        QVERIFY(cleaned.toUtf8().size() <= MAX_FILE_NAME_BYTES);
        QVERIFY(cleaned.endsWith(".txt"));
        QVERIFY(!cleaned.startsWith('.'));

        // a name without an extension is just cut down
        const QString no_ext = FileTransferMgr::SanitizeFileName(QString(200, QChar('y')));
        QVERIFY(no_ext.toUtf8().size() <= MAX_FILE_NAME_BYTES);
        QCOMPARE(no_ext, QString(MAX_FILE_NAME_BYTES, QChar('y')));
    }

    void sendFileReportsWhyItRejected()
    {
        QString error = QStringLiteral("unchanged");
        QVERIFY(FileTransferMgr::GetInstance()
                    ->SendFile(5, "/nonexistent/definitely/missing.bin", &error)
                    .isEmpty());
        QCOMPARE(error, QStringLiteral("文件不存在"));

        error.clear();
        QVERIFY(FileTransferMgr::GetInstance()->SendFile(0, "/tmp/x", &error).isEmpty());
        QCOMPARE(error, QStringLiteral("对方不在线"));
    }

    void formatSizePicksAReadableUnit()
    {
        QCOMPARE(FileBubble::FormatSize(0), QString("0 B"));
        QCOMPARE(FileBubble::FormatSize(999), QString("999 B"));
        QCOMPARE(FileBubble::FormatSize(1024), QString("1.0 KB"));
        QCOMPARE(FileBubble::FormatSize(1024 * 1024), QString("1.0 MB"));
        QCOMPARE(FileBubble::FormatSize(1024LL * 1024 * 1024), QString("1.0 GB"));
        QCOMPARE(FileBubble::FormatSize(1536), QString("1.5 KB"));
    }
};

QObject* createFileTransferHelpersTest()
{
    return new FileTransferHelpersTest;
}

#include "test_filetransfer.moc"
