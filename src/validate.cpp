#include "validate.h"
#include <qboxlayout.h>
#include <qdialogbuttonbox.h>
#include <qgroupbox.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qscrollarea.h>

void drawValidationWindow(QWidget *parent){
    QDialog dialog(parent);
    dialog.setWindowTitle("Validation");
    dialog.resize(800, 600);

    QHBoxLayout *mainLayout = new QHBoxLayout(&dialog);

    QVBoxLayout *validationSelection = new QVBoxLayout();
    mainLayout->addLayout(validationSelection, 4);

    QVBoxLayout *recordAffected = new QVBoxLayout();
    mainLayout->addLayout(recordAffected, 6);

    //validation options selection
    validationSelection->addWidget(new QLabel("Record Validation Options"));

    QScrollArea *selectionScrollArea = new QScrollArea();
    validationSelection->addWidget(selectionScrollArea);

    QWidget *selectionScrollAreaContent = new QWidget();
    QVBoxLayout *selectionContentLayout = new QVBoxLayout(selectionScrollAreaContent); 
    selectionScrollArea->setWidget(selectionScrollAreaContent);
    
    QGroupBox *v1Record = new QGroupBox();
    selectionContentLayout->addWidget(v1Record);

    QDialogButtonBox *buttons = new QDialogButtonBox();

    QPushButton *validateButton = buttons->addButton("Validate", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);

    mainLayout->addStretch();
    mainLayout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

