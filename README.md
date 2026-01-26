# Podcast Createur

**Podcast Createur** est une application de bureau développée en C++ avec le framework Qt 6. Elle permet de créer, enregistrer, éditer et assembler des épisodes de podcast de manière intuitive.

L'application est conçue pour gérer un projet d'émission sous forme de "chroniques" (segments) que l'on peut déplacer, éditer individuellement et fusionner en un fichier final.

## 🚀 Fonctionnalités

### Gestion de Projet
- Création et gestion de multiples projets (émissions) sauvegardés localement.
- Interface d'accueil listant les émissions récentes.

### Timeline (Conducteur)
- Ajout de chroniques (segments audio).
- **Drag & Drop** : Réorganisation de l'ordre des chroniques par glisser-déposer.
- Indicateurs visuels d'état (Script présent / Audio présent).
- Import de fichiers audio externes (MP3, WAV, M4A, FLAC).

### Enregistrement (Studio)
- Enregistreur intégré.
- **Prompteur** : Affichage du script textuel pendant l'enregistrement.
- Sélection du périphérique d'entrée (microphone).

### Éditeur Audio
- Visualisation de la forme d'onde (Waveform).
- **Fonctions d'édition** : Couper, Normaliser (gain auto), Zoom.
- Gestion de la sélection à la souris.
- Pré-écoute (Play/Pause/Stop).

### Export
- Fusion (Mixage) de toutes les chroniques en un seul fichier MP3 final.
- Utilisation de FFmpeg pour garantir la qualité et la compatibilité.

## 🛠 Prérequis Techniques

Pour compiler et exécuter ce projet, vous avez besoin de :

1.  **Qt 6.x** (Modules requis : `Core`, `Gui`, `Widgets`, `Multimedia`).
2.  **Compilateur C++17** compatible (MSVC 2019+, GCC, Clang).
3.  **FFmpeg** : L'application dépend des exécutables `ffmpeg` pour l'importation de formats compressés et l'export final.

## 📦 Installation et Compilation

### 1. Cloner ou télécharger les sources
Assurez-vous d'avoir tous les fichiers sources (`.cpp`, `.h`, `.ui`, `.pro`, `.qrc`).

### 2. Ouvrir avec Qt Creator
Ouvrez le fichier `PodcastCreatorQt.pro` dans Qt Creator.

### 3. Compilation
Lancez la compilation (Build).

### 4. Configuration de FFmpeg (CRITIQUE)
L'application ne fonctionnera pas correctement sans FFmpeg.

*   **Sous Windows** :
    1.  Téléchargez les exécutables `ffmpeg.exe` (depuis [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) par exemple).
    2.  Placez ces fichiers **dans le même dossier que l'exécutable compilé** (`release` ou `debug`), ou ajoutez-les à votre PATH système.

*   **Sous macOS** :
    1.  Installez FFmpeg via Homebrew : `brew install ffmpeg`.
    2.  Ou placez l'exécutable `ffmpeg` à l'intérieur du bundle de l'application (`PodcastCreator.app/Contents/MacOS/`).

*   **Sous Linux** :
    1.  Installez FFmpeg via votre gestionnaire de paquets : `sudo apt install ffmpeg`.

## 📖 Guide d'utilisation

1.  **Accueil** : Lancez l'application. Cliquez sur "Nouvelle Émission" et donnez-lui un nom.
2.  **Ajout de segments** : Cliquez sur "+ Chronique" pour ajouter une section (ex: Intro, Interview, Conclusion).
3.  **Écriture** : Cliquez sur l'icône "Script" d'une chronique pour écrire votre texte.
4.  **Enregistrement** : 
    *   Si aucun audio n'existe, le bouton principal est un Micro. Cliquez pour ouvrir le Studio.
    *   Lisez votre texte tout en enregistrant.
5.  **Édition** :
    *   Une fois l'audio présent, le bouton devient "Lecture".
    *   Faites un clic droit (ou via le menu `...`) -> "Éditer l'audio".
    *   Sélectionnez une zone de silence ou de bruit avec la souris, puis cliquez sur "Couper".
    *   Sélectionnez une zone faible et cliquez sur "Normaliser" pour amplifier le son.
    *   Sauvegardez.
6.  **Organisation** : Maintenez le clic sur la poignée (à gauche de la chronique) pour changer l'ordre des segments.
7.  **Finalisation** : Cliquez sur le bouton vert **"Fusionner"** en haut. Le fichier final sera généré dans votre dossier "Musique".

## 📂 Structure du Code

*   **`main.cpp`** : Point d'entrée, gestion de la boucle entre l'accueil et la fenêtre projet.
*   **`homedialog`** : Fenêtre de gestion des projets.
*   **`projectwindow`** : Fenêtre principale (Timeline), gère la liste des chroniques.
*   **`chroniclewidget`** : Widget personnalisé représentant une ligne (carte) dans la timeline.
*   **`audiorecorder`** : Dialogue d'enregistrement avec affichage du script.
*   **`audioeditor`** : Fenêtre d'édition audio (logique de lecture, buffer audio, appel FFmpeg).
*   **`waveformwidget`** : Widget de dessin personnalisé (QPainter) pour afficher l'onde sonore.
*   **`audiomerger`** : Classe utilitaire gérant le processus `QProcess` pour appeler FFmpeg lors de la fusion.

## 📄 Licence

Ce projet est fourni tel quel à des fins éducatives ou personnelles.
Copyright © 2025 Podcast Creator.