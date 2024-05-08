#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>

#include <QStringList>
#include <QListWidget>
#include <configmaps/ConfigMap.hpp>

#include "XRockGUI.hpp"

namespace xrock_gui_model
{

    class LinkHardwareSoftwareDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit LinkHardwareSoftwareDialog(XRockGUI *xrockGui, configmaps::ConfigMap &softwareMap);

        configmaps::ConfigMap& getSoftwareMap() { return softwareMap; }

    private slots:
        void removeSelectedConfirgured_model();
        void addConfiguredModels();
        void closeEvent(QCloseEvent *event) override;
        

    private: 
        XRockGUI *xrockGui;
        configmaps::ConfigMap softwareMap;

        QVBoxLayout *layout;
        QListWidget *listWidget;

        QPushButton *minusBtn;
        QPushButton *plusBtn;

    };
}