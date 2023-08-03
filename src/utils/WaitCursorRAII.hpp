#pragma once
#include <QApplication>
#include <QCursor>

class WaitCursorRAII
{
public:
    WaitCursorRAII()
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    ~WaitCursorRAII()
    {
        QApplication::restoreOverrideCursor();
    }
};
