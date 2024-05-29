#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QStringList>


namespace xrock_gui_model
{
    class BundleSelectionDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit BundleSelectionDialog(QWidget *parent = nullptr);
        QString getSelectedBundle() const;

    private slots:
        void acceptButtonClicked();

    private:
        QVBoxLayout *layout;
        QList<QRadioButton *> radioButtons;
        QString selectedBundle;

        void populateBundles();
    };
}