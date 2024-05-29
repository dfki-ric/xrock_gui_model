#include "LinkHardwareSoftwareDialog.hpp"
#include <QDir>
#include <cstdlib>
#include <QMessageBox>
#include <iostream>
#include <mars/utils/misc.h>
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"
#include "ImportDialog.hpp"
#include "utils/WaitCursorRAII.hpp"

using namespace configmaps;

namespace xrock_gui_model
{
    LinkHardwareSoftwareDialog::LinkHardwareSoftwareDialog(XRockGUI *xrockGui, configmaps::ConfigMap& softwareMap)
        : QDialog(), xrockGui(xrockGui), softwareMap(softwareMap), layout(new QVBoxLayout(this))
    {

        setWindowTitle(QString::fromStdString("Manage " + (std::string)softwareMap["name"] + " Hardware Links"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto label = new QLabel("Configured for:");
        layout->addWidget(label);

        // list already configured hardwares
        listWidget = new QListWidget();
        layout->addWidget(listWidget);

        auto hbox = new QHBoxLayout();
        minusBtn = new QPushButton("-");
        minusBtn->setToolTip("Remove selected hardware");
        connect(minusBtn, SIGNAL(clicked(bool)), this, SLOT(removeSelectedConfiguredModel()));
        hbox->addWidget(minusBtn);

        plusBtn = new QPushButton("+");
        plusBtn->setToolTip("Add new hardware");
        connect(plusBtn, SIGNAL(clicked(bool)), this, SLOT(addConfiguredModels()));
        hbox->addWidget(plusBtn);

        layout->addLayout(hbox);


        // Make sure relation is initialized to empty if doesnt exist
        if(!softwareMap.hasKey("configured_for"))
            softwareMap["configured_for"] = ConfigVector();

        // Fill list with existing configured_for hardware models
        for (auto &assemblyModel : softwareMap["configured_for"])
        {
            std::string name = assemblyModel["name"];
            std::string version = assemblyModel["version"];
            QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(name + +" ( " + version + " ) "));
            QVariant tag;
            tag.setValue(QString::fromStdString(assemblyModel["uri"]));
            item->setData(Qt::UserRole, tag);
            listWidget->addItem(item);
        }
    }

    void LinkHardwareSoftwareDialog::addConfiguredModels()
    {
        // Pick cm from db
        //ImportDialog *id = new ImportDialog(xrockGui, ImportDialog::Intention::SELECT_HARDWARE);
        ImportDialog id(xrockGui, ImportDialog::Intention::SELECT_HARDWARE);
        id.exec();

        // what if the user didnot select any thing from import dialg?.
        auto assemblyModel = id.getModel();
        if(assemblyModel.empty())
            return;

        // 1. add its name - version it to qlistwidget
        std::string assemblyModelName = assemblyModel["name"];
        std::string version = assemblyModel["versions"][0]["name"];
        std::string deployUri = softwareMap["uri"].getString();
        bool already_linked = false;
        for (auto a : softwareMap["configured_for"])
        {
            if (a["uri"] == assemblyModel["uri"])
            {
                already_linked = true;
                break;
            }
        }
        if (already_linked)
        {
            QMessageBox::warning(nullptr, "Warning", QString("The deployment of '%1' is already linked to  hardware '%2'.")
                                         .arg(QString::fromStdString(softwareMap["name"].getString()))
                                         .arg(QString::fromStdString(assemblyModelName), QMessageBox::Ok));
            return;
        }
            // We add it to basic model
            ConfigMap newConfiguredFor;
            newConfiguredFor["name"] = assemblyModelName;
            newConfiguredFor["version"] = version;
            newConfiguredFor["uri"] = (std::string)assemblyModel["uri"];
            softwareMap["configured_for"].push_back(newConfiguredFor);

            // Alles gut, add hw to gui
            QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(assemblyModelName + " ( " + version + " ) "));
            QVariant tag;
            tag.setValue(QString::fromStdString(assemblyModel["uri"]));
            item->setData(Qt::UserRole, tag);
            listWidget->addItem(item);

            QMessageBox::information(nullptr,
                                     "Success",
                                     QString("The deployment of '%1' with hardware '%2' is linked.\n\nDon't forget to save the component model '%1' to the database.")
                                         .arg(QString::fromStdString(softwareMap["name"].getString()))
                                         .arg(QString::fromStdString(assemblyModelName)),
                                     QMessageBox::Ok);
        
    }

    void LinkHardwareSoftwareDialog::removeSelectedConfiguredModel()
    {
        QListWidgetItem *item = listWidget->currentItem();
        if (item)
        {
            if (QMessageBox::question(nullptr, "Question",  "Are you sure you want to remove the selected hardware ?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) == QMessageBox::Yes)
            {
                QVariant tag = item->data(Qt::UserRole);
                std::string hardware_uri = tag.toString().toStdString();

                // Delete it from softwareMap
                auto &configured_for = ((ConfigVector &)softwareMap["configured_for"]);
                configured_for.erase(std::remove_if(configured_for.begin(), configured_for.end(), [&hardware_uri](auto &item)
                                                    { return (std::string)item["uri"] == hardware_uri; }),
                                     configured_for.end());

                // will delete the list item from the qlistwidget
                delete item;
            
                QMessageBox::information(nullptr, "Success", "Hardware has been successfully unlinked from software", QMessageBox::Ok);
            }
        

        }
    }
    void LinkHardwareSoftwareDialog::closeEvent(QCloseEvent *event)
    {
        static bool remindUser = false;
        if(!remindUser) { // only show once per session
            QMessageBox::information(this, tr("Confirm Close"), tr("Don't forget to save the changes to database"), QMessageBox::Ok);
            remindUser = true;
        }

    }
}