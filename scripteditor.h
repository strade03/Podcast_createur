#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include <QDialog>
#include <QTextEdit>

class ScriptEditor : public QDialog
{
    Q_OBJECT
public:
    explicit ScriptEditor(const QString &filePath, QWidget *parent = nullptr);

private slots:
    void save();

private:
    QString m_path;
    QTextEdit *editor;
};

#endif // SCRIPTEDITOR_H