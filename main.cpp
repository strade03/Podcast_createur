#include "homedialog.h"
#include "projectwindow.h"
#include "audiomerger.h"
#include <QApplication>
#include <QEventLoop>

// Includes pour le préchauffage
#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("MonStudio");
    a.setOrganizationDomain("monstudio.com");
    a.setApplicationName("Podcast Createur");
    a.setApplicationVersion(APP_VERSION); 

    // ============================================================
    // ZONE DE PRÉCHAUFFAGE (WARM-UP)
    // On charge les modules lourds maintenant pour que l'antivirus 
    // fasse son scan au lancement, et pas pendant l'utilisation.
    // ============================================================
    {
        // 1. Charger le module GUI complexe (QMainWindow)
        QMainWindow dummyWindow;
        dummyWindow.setAttribute(Qt::WA_DontShowOnScreen);
        dummyWindow.show(); // On force l'init graphique
        dummyWindow.close();

        // 2. Charger le module Multimédia (Le plus suspect pour les antivirus)
        // Cela charge les backends audio (Wmf, DirectShow, etc.)
        // QMediaPlayer dummyPlayer;
        // QAudioOutput dummyOutput;
        // dummyPlayer.setAudioOutput(&dummyOutput);
        
        // 3. Init FFmpeg (déjà présent mais on le garde ici)
        AudioMerger::init();
        
        // On laisse le temps à l'OS de "digérer" ces chargements
        QCoreApplication::processEvents();
    }
    // ============================================================
    // FIN DU PRÉCHAUFFAGE
    // ============================================================

    while (true) {
        // 1. Ouvrir l'accueil
        HomeDialog home;
        if (home.exec() != QDialog::Accepted) {
            break;
        }

        // 2. Récupérer le choix
        QString name = home.getSelectedProjectName();
        QString path = home.getSelectedProjectPath();

        // 3. Lancer la fenêtre projet
        ProjectWindow *mainWindow = new ProjectWindow(name, path);
        mainWindow->show();

        // 4. Boucle d'attente
        QEventLoop loop;
        QObject::connect(mainWindow, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
        loop.exec();
    }

    return 0;
}

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);
//     a.setApplicationName("Podcast Createur");
//     a.setApplicationVersion(APP_VERSION); // défini dans PodcastCreatorQt.pro

//     AudioMerger::init();
//     while (true) {
//         // 1. Ouvrir l'accueil
//         HomeDialog home;
//         if (home.exec() != QDialog::Accepted) {
//             // Si l'utilisateur annule ou ferme la croix, on quitte tout
//             break;
//         }

//         // 2. Récupérer le choix
//         QString name = home.getSelectedProjectName();
//         QString path = home.getSelectedProjectPath();

//         // 3. Lancer la fenêtre projet
//         // On utilise un pointeur pour gérer sa durée de vie manuellement dans la boucle
//         ProjectWindow *mainWindow = new ProjectWindow(name, path);
//         mainWindow->show();

//         // 4. Créer une boucle d'événement locale pour attendre la fermeture de la fenêtre
//         QEventLoop loop;
//         QObject::connect(mainWindow, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
        
//         // Cette ligne bloque tant que la fenêtre est ouverte
//         loop.exec();

//         // Une fois fermée, on boucle et on réouvre HomeDialog
//         // (La mémoire de mainWindow est libérée par son attribut WA_DeleteOnClose)
//     }

//     return 0;
// }

// #include "homedialog.h"
// #include "projectwindow.h"
// #include "audiomerger.h"
// #include <QApplication>
// #include <QTimer>

// // Fonction pour afficher l'accueil et lancer un projet
// void showHome(QApplication &app);

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);
//     a.setApplicationName("Podcast Creator");
//     a.setApplicationVersion("2.2");

//     // Initialisation FFmpeg une seule fois au démarrage
//     AudioMerger::init();
    
//     // Lancer l'accueil
//     showHome(a);

//     // Lancer la boucle d'événements Qt
//     return a.exec();
// }

// void showHome(QApplication &app)
// {
//     // 1. Créer et afficher l'accueil
//     HomeDialog *home = new HomeDialog();
    
//     // 2. Quand l'utilisateur choisit un projet
//     QObject::connect(home, &QDialog::accepted, [home, &app]() {
//         QString name = home->getSelectedProjectName();
//         QString path = home->getSelectedProjectPath();
        
//         // 3. Créer la fenêtre projet
//         ProjectWindow *mainWindow = new ProjectWindow(name, path);
        
//         // 4. Quand la fenêtre projet se ferme, réafficher l'accueil après 100ms
//         QObject::connect(mainWindow, &QMainWindow::destroyed, [&app]() {
//             // Délai pour éviter de déclencher l'antivirus
//             QTimer::singleShot(100, [&app]() {
//                 showHome(app);
//             });
//         });
        
//         mainWindow->show();
//         home->deleteLater(); // Libérer la mémoire de l'accueil
//     });
    
//     // 5. Si l'utilisateur ferme l'accueil, quitter l'application
//     QObject::connect(home, &QDialog::rejected, [&app, home]() {
//         home->deleteLater();
//         app.quit();
//     });
    
//     home->show();
// }