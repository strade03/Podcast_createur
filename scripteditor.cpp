#include "scripteditor.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

ScriptEditor::ScriptEditor(const QString &filePath, QWidget *parent)
    : QDialog(parent), m_path(filePath)
{
    setWindowTitle("Éditeur de Script");
    resize(400, 500);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    editor = new QTextEdit(this);
    layout->addWidget(editor);
    
    // Charger
    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        // in.setCodec("UTF-8"); // Qt6 est UTF8 par defaut
        editor->setText(in.readAll());
        f.close();
    }
    
    QPushButton *btnSave = new QPushButton("Sauvegarder et Fermer", this);
    connect(btnSave, &QPushButton::clicked, this, &ScriptEditor::save);
    layout->addWidget(btnSave);
}

void ScriptEditor::save()
{
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << editor->toPlainText();
        f.close();
        accept();
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire le fichier");
    }
}