# État — mise à jour 2026-11-13 (après-midi)

## Réponse courte

**Oui — une partie solo en français se joue de bout en bout.** Vérifié par capture
interne de la fenêtre (`QQuickWindow::grabWindow`, plus fiable que les captures écran) :
nouvelle partie → chevalet tiré → « Compute Move » (550 mots trouvés) → ELAVEES posé
pour 72 points → joueur suivant → chevalet complété (88 lettres dans le sac), messages
affichés dans le volet droit, barre d'état remplie.

## Ce qui a été corrigé cette session

- **Le volet droit et la barre d'état n'étaient pas cassés** : le diagnostic précédent
  venait des captures écran (fenêtre hors écran / rendu matériel). Captés depuis l'app,
  ils s'affichent ; le volet est simplement vide tant qu'aucun message n'est émis.
- **Aucun dictionnaire n'était chargé dans l'app** (`dictionary=` vide, jeu de lettres
  anglais) → « No word found ». Config passée en jeu de lettres *French* + `francais.dic`
  et sauvegardée dans `%LOCALAPPDATA%\Scrabble3D\4\Scrabble3D.ini` (sauvegarde de
  l'ancien .ini dans le scratchpad de la session). Le dictionnaire (407 130 mots) se
  charge maintenant au démarrage.
- `ScrConfig.qml` : `SafeArea` (API Qt ≥ 6.9) protégé par
  `parent.SafeArea ? … : 0` → plus d'erreurs `TypeError … 'margins'`.

## Comparaison avec la v3 (captures Malavida, version 3.1.4 de 2015)

Les captures de https://scrabble-3d.fr.malavida.com/windows/ montrent l'ancienne v3,
pas cette v4 Qt6 : même principe (plateau 2D à gauche ; à droite messages, déroulé de
la partie, marqueur ; barre d'état joueur/score/sac). En v4 le chevalet est sous le
plateau. Onglet « Game course » vérifié : 2 coups (DRALON 18, MORDORAS 70) affichés
avec valeur, meilleur score et durée ; seuls les en-têtes de colonnes sont tronqués
(« Val… », « B… », « Ti… »). Un utilisateur Malavida signalait déjà que le dictionnaire
français ne se chargeait pas : c'est le bug CRLF corrigé dans `dictionary.cpp`.
Le dépôt amont (github.com/Scrabble3D/4.0) n'a ni release Windows ni instructions de
build ; `QScrabble.pro` ne fixe pas de version de Qt.

## Constat important

Le code amont vise **Qt ≥ 6.10** (`SafeArea` 6.9, `SortFilterProxyModel` /
`RoleSorter` de QtQml.Models 6.10). Avec Qt 6.8.3 on patche au cas par cas ; une
alternative propre serait d'installer Qt 6.10 (aqtinstall) et de revenir aux QML d'origine.

## Améliorations (branche `ameliorations-qt68`, 4 commits, non poussée)

- **Partie contre l'ordinateur par défaut** : le dialogue « New Game » propose 2 joueurs,
  le second coché « Computer » (`ScrNewGame.qml`) ; le .ini local a aussi été passé à
  `playerCount=2`, `comp2=true`. Vérifié : l'humain pose RHODIE (28), l'ordinateur répond
  automatiquement BLOOMER (40), puis la main revient à l'humain.
- **Glisser-déposer vérifié** avec de vrais événements souris (QtTest, lié seulement le
  temps du test) : deux lettres glissées du chevalet arrivent sur le plateau.
- **En-têtes du déroulé de partie lisibles** (`# Word Value Best Time`, plus de « Val… »).
- **`rackmodel.cpp`** : `qFatal("Invalid rackmodel request")` remplacé par un
  avertissement + borne correcte (`>=` au lieu de `>`, index de ligne vérifié).
- **`Component is not ready` supprimé** : le `Loader` principal de `mainwindow.qml` est
  passé en synchrone (le changement d'état paysage/portrait relançait l'incubation async).

## Reste à faire

1. Cube 3D toujours désactivé (`ScrCube.qml`) — à réactiver avec Qt 6.10 + QtQuick3D.
2. Tri des colonnes du déroulé / des parties distantes perdu (pas de `SortFilterProxyModel`
   en Qt 6.8).
3. Un mot invalide posé par l'humain puis « Next Player » ouvre une confirmation : à
   vérifier à la main.

---

# Historique — état précédent (matin)

## Réponse courte

**Non — on ne peut pas encore jouer sans problème.** L'application compile, démarre,
charge le dictionnaire français et lance une nouvelle partie, mais **l'interface de jeu
n'est pas encore jouable** : le volet de droite (messages, onglets, joueurs) et la barre
d'état ne s'affichent pas.

## Ce qui marche

- L'app démarre, le plateau 15×15 s'affiche.
- Le dictionnaire français se charge (407 130 mots, vérifié via la vraie classe `dictionary`).
- Une nouvelle partie se crée sans crash : 7 lettres tirées au sort (ex. `Y A N A O E T`)
  et affichées dans le chevalet.
- Correction d'un point bloquant : `rackmodel.cpp:158` faisait un `qFatal` (arrêt brutal
  de l'app) sur une requête de modèle invalide ; transformé en avertissement — à valider.

## Ce qui ne marche pas encore

- Le volet de droite (messages, onglets, joueurs) reste **vide**.
- La barre d'état **ne s'affiche pas**.
  - Vérifié : la géométrie est correcte (volet 570×706, barre 1536×24), mais leur
    contenu ne se peint pas.
- Pas de main / score / dialogue de tour visibles → impossible de jouer réellement.
- Le cube 3D est désactivé (module QtQuick3D déployable incomplet dans cette installation).

## Cause probable des captures trompeuses

La fenêtre est plus haute que la zone écran (1550×938 pour 928 px utiles) et le rendu
matériel ne se rafraîchit pas pour `PrintWindow`. Contournement utilisé : backend
logiciel (`QT_QUICK_BACKEND=software`) + redimensionnement de la fenêtre.

Malgré cela, le volet droit reste vide : c'est un vrai problème de layout QML/Qt6, pas
seulement un artefact de capture.

## Reste à faire pour « jouer sans problème »

1. Comprendre pourquoi `ColumnLayout` / `TableView` / `RowLayout` du volet droit ne
   rendent rien (items à la bonne taille mais sans affichage).
2. Vérifier le dialogue « New Game » et les interactions souris (poser/déposer une
   lettre, valider avec le dictionnaire).
3. Réactiver ou abandonner proprement le cube 3D.

## État du dépôt

Fichiers modifiés (rien n'est commité) :

| Fichier | Changement |
|---|---|
| `dictionary.cpp` | **Vrai correctif** : gestion des fins de ligne CRLF (le dictionnaire se charge enfin). |
| `ScrRemoteGames.qml` | Patch Qt6 : `SortFilterProxyModel`/`RoleSorter`/`HorizontalHeaderViewDelegate` remplacés. |
| `ScrGameCourse.qml` | Idem. |
| `ScrMessages.qml` | Proxy de tri supprimé (les messages s'ajoutent en bas). |
| `ScrCube.qml` | `View3D` remplacé par un `Item` vide (QtQuick3D indisponible). |
| `main.cpp` | `QT_QUICK_CONTROLS_STYLE=Basic` pour supprimer les avertissements de style. |

## Environnement

- Qt 6.8.3 `win64_llvm_mingw` + LLVM-MinGW 22.1.7 dans `C:\Qt`.
- Build : `C:\Users\DELL\Projets\Scrabble3D\build` (qmake + `mingw32-make -f Makefile.Release`).
- Exécutable déployé : `build/release/Scrabble3D.exe`.
- Config / dictionnaires : `C:\Users\DELL\AppData\Local\Scrabble3D\4`.
