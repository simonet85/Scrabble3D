# Étapes suivantes — Scrabble3D

État au 2026-11-13 : `main` contient le portage Qt 6.8 (PR #1), le parcours joueur (PR #2)
et les scores + plateau façon plateau imprimé (PR #3). Une partie complète Humain vs
Computer se joue de bout en bout.

Par ordre d'intérêt :

## 1. Interface en français (rapide, gros gain)

- Tout est encore en anglais (« Pass », « TRIPLE WORD », « It's Computer's turn »…).
- Le jeu sait déjà télécharger et charger une traduction (`GamePlay::localize`,
  `locmodel`) : activer la traduction française.
- Traduire ensuite les textes ajoutés dans les PR #2 et #3 (boutons du chevalet,
  noms des cases bonus, « Winner: »), que la traduction existante ne connaît pas.

## 2. Fiabiliser ce qui reste fragile

- **Blocages isolés au démarrage** : après le remplacement du glyphe « ★ » par une image,
  2 gels isolés puis 34 démarrages réussis. Cause non identifiée : refaire une série de
  démarrages pour voir s'ils reviennent.
- **Mot invalide posé par un humain** : la confirmation « Unknown word(s) — accept anyway? »
  (`GamePlay::doCheckWords`, `QMessageBox`) n'a jamais été testée.
- **Tests automatiques permanents** : transformer le hook de test temporaire (évaluation
  JS, clavier/souris QtTest, capture `grabWindow`) en vrai petit jeu de tests qui rejoue
  une partie complète à chaque modification.

## 3. Rendre l'application installable

- Aujourd'hui elle ne se lance que depuis `build/release` avec Qt dans le `PATH`.
- Dossier autonome avec `windeployqt` + raccourci, voire un installateur, pour l'utiliser
  sans l'environnement de développement.

## 4. Récupérer ce que Qt 6.8 a fait perdre (gros chantier)

- Installer Qt 6.10 (aqtinstall) et revenir aux QML d'origine
  (`SortFilterProxyModel`, `SafeArea`, `View3D`).
- On récupère le **cube 3D** et le **tri des colonnes** du déroulé de partie et des
  parties distantes.
- Gros téléchargement et risques de régression à retester.

## 5. Améliorations de jeu possibles

- Niveau de l'ordinateur réglable (débutant / fort) : il joue toujours le meilleur coup.
- Bouton « Indice » qui montre un bon coup sans le jouer.
- Tester le jeu en réseau, jamais essayé.

## 6. Ménage

- Supprimer les branches déjà fusionnées : `ameliorations-qt68`, `parcours-joueur`,
  `scores-et-plateau`.
- Mettre à jour `STATUS.md`, resté à l'état d'avant les PR #2 et #3.

**Recommandation** : commencer par le 1 (français), puis le 3 (installable).
