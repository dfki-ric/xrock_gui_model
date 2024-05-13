#include "BundleSelectionDialog.hpp"
#include <QDir>
#include <cstdlib>
#include <QMessageBox>
#include <iostream>
#include <mars/utils/misc.h>

namespace xrock_gui_model
{
    BundleSelectionDialog::BundleSelectionDialog(QWidget *parent)
        : QDialog(parent), layout(new QVBoxLayout(this)), selectedBundle("")
    {
        populateBundles();

        QPushButton *okButton = new QPushButton("OK", this);
        connect(okButton, SIGNAL(clicked()), this, SLOT(acceptButtonClicked()));
        layout->addWidget(okButton);
    }

    void BundleSelectionDialog::populateBundles()
    {
        const char *rockBundlePath = std::getenv("ROCK_BUNDLE_PATH");
        
        if (rockBundlePath != nullptr)
        {
            std::string bundlesPath = mars::utils::explodeString(':', std::string(rockBundlePath))[0];

            QDir dir(QString::fromStdString(bundlesPath));
            QStringList bundles = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            foreach (const QString &bundle, bundles)
            {
                QRadioButton *radioButton = new QRadioButton(bundle, this);
                radioButtons.append(radioButton);
                layout->addWidget(radioButton);
            }
        } else {
            QMessageBox::warning(this, "Warning", "Missing env ROCK_BUNDLE_PATH", QMessageBox::Ok);
        }
    }

    QString BundleSelectionDialog::getSelectedBundle() const
    {
        return selectedBundle;
    }

    void BundleSelectionDialog::acceptButtonClicked()
    {
        foreach (QRadioButton *radioButton, radioButtons)
        {
            if (radioButton->isChecked())
            {
                selectedBundle = radioButton->text();
                break;
            }
        }
        if (selectedBundle.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Please select a bundle.", QMessageBox::Ok);
        }
        else
        {
            accept();
        }
    }
}