# Tâches Scrabble3D

`[x]` = fait et testé, `[ ]` = à faire. Les numéros 1 à 14 sont ceux de la liste de fonctionnalités proposée
(les plus utiles d'abord).

## Liste proposée (1 à 14)

### Forte valeur, peu long
- [x] **1. Bilan de partie par le coach.** Le meilleur coup possible est enregistré discrètement au début de chacun de vos
  tours (sans rien afficher, sans indice possible). À la fin de la partie, le bilan compare vos coups aux meilleurs :
  points marqués / possibles, coups parfaits, trois plus grosses pertes, puis un commentaire du coach (si une clé est
  configurée). Onglet Coach, menu Partie > Bilan de partie. Option pour couper l'enregistrement.
- [x] **2. Bouton « Indice »** (sans IA). Le premier indice d'un tour montre où un bon coup peut être joué (points, nombre
  de tuiles, case de départ surlignée en jaune, sens) ; un deuxième au même tour montre le mot et surligne toutes ses
  cases. Aucun coup n'est joué ni conservé (pas de liste de mots). 3 indices par partie, réglable de 0 à 20
  (Configuration > Règles). Bouton à côté du chevalet et menu Partie > Indice ; désactivé en réseau, au tour de
  l'ordinateur et en 3D. Le bilan de partie indique les indices utilisés.
- [x] **3. Réglage du son** : case « Son lors d'un Scrabble », volume (jingle généré en mémoire, donc vrai réglage de
  volume), bouton Tester. Page Configuration > Plateau.
- [x] **4. Défense au niveau Maître.** Un coup qui rend accessibles à l'adversaire des cases « mot compte » encore
  libres (une tuile à 4 cases ou moins sur la même ligne ou colonne) est pénalisé : 10 points pour un triple,
  3 pour un double. Le Maître, l'indice et le coup recommandé par le coach utilisent la même règle ; le coach sait
  pour chaque coup s'il ouvre une case prime. Sur une partie de 12 tours : risque cumulé 96 → 25 points, pour
  5 points de moins marqués (342 contre 347).
- [x] **5. Messages en français** : tous les textes ajoutés au jeu (99) ont une traduction française intégrée, qui
  complète la traduction téléchargée sans la remplacer (`frenchsupplement`).

### Règlement FISF 2026 (restant)
- [x] **6. Temps épuisé (§5.1.3).** En temps par partie, le joueur qui n'a plus de temps passe automatiquement à
  chacun de ses tours, après la période de contestation du dernier coup (il peut donc encore contester). Un message
  l'annonce une seule fois. Ses adversaires ne peuvent plus échanger de lettres (l'ordinateur non plus). Les tuiles
  posées au moment où le temps s'écoule sont rendues au chevalet (§4.5). En fin de partie, la valeur des chevalets
  adverses n'est ajoutée qu'au joueur qui a vidé son chevalet (§5.1.1) ; une partie finie par des passes ne fait que
  retirer à chacun la valeur de son chevalet. Corrigé aussi : un minuteur en double pouvait faire compter le temps
  deux fois.
- [x] **7. Barème de sanctions (§6.4).** Menu Partie → « Sanction... » : on choisit le joueur et on note
  l'infraction ; le dialogue annonce la sanction qui s'applique. Barème par joueur : rappel du règlement, 5 points,
  50 points, puis match perdu (score à 0, la partie se termine). Chaque sanction est annoncée dans les messages ;
  les compteurs repartent à zéro à chaque nouvelle partie. Pas en partie réseau. Les sanctions ne sont pas
  enregistrées avec la partie sauvegardée.
- [ ] 7 bis. Export de la feuille de match (PDF ou CSV) — laissé de côté pour l'instant.
- [x] **8. Déclaration du joker (§3.1.4).** Le sélecteur oblige déjà à choisir la lettre du joker ; chaque joker
  posé est maintenant annoncé (« X joue un joker comme S dans LYSEZ (H10) ») et noté avec le coup, le joueur et
  la case. En fin de partie (normale, abandon ou match perdu), le relevé des jokers suit la feuille de match. Un
  coup retiré après contestation retire aussi ses jokers.

### Coach et dictionnaire
- [x] **9. Onglet Coach en mise en page mobile.** La mise en page portrait a son onglet Coach (avec le point rouge
  des nouveaux conseils) ; les actions du coach l'ouvrent comme sur ordinateur. Le panneau sait désormais s'il est
  vraiment affiché : dans le SwipeView du portrait, les pages restent « visibles » même hors écran.
- [x] **10. Mise à jour du dictionnaire.** Menu Aide → « Rechercher des mises à jour » : le jeu lit la liste
  officielle du serveur et propose les dictionnaires et traductions plus récents, ou dit que tout est à jour (la
  vérification automatique reste hebdomadaire). Les mots importés sont gardés dans `<dictionnaire>.imported.txt` :
  après une mise à jour du dictionnaire, ils sont ajoutés de nouveau à la nouvelle édition.
- [ ] 11. Clés d'API chiffrées (gestionnaire d'identifiants de Windows) au lieu de `coach.ini` en clair.

### Réseau et technique
- [ ] 12. Connexion chiffrée au serveur ; abandon et tirage au sort en partie réseau.
- [x] **13. Paquet Windows.** `deploy\package-windows.ps1` compile en release, rassemble Qt avec `windeployqt`
  dans `dist\Scrabble3D\` (110 Mo) et en fait un zip (40 Mo) qui se lance sans Qt installé. Si Inno Setup 6 est
  installé, il produit aussi `Scrabble3D-<version>-setup.exe` (`deploy\scrabble3d.iss`, en français et anglais).
- [x] **14. Erreur QML `ScrBoard.qml:75`.** Les couleurs étaient lues avant d'être chargées : `config.fieldColor()`
  et `config.playerColor()` renvoient un gris neutre en attendant, puis la vraie couleur. Les tests ne filtrent
  plus cette erreur.

## Déjà fait

### Règlement international du Scrabble classique (version du 1er septembre 2026)
- [x] Français par défaut : jeu de lettres, dictionnaire `francais.dic`, contestation (20 s, 5 points) comme préréglage.
- [x] Bonus Scrabble par tuile supplémentaire, +25 au-delà de 7 (§3.1.2c).
- [x] Contestation : pénalité de 5 points par mot contesté à tort (§3.5.3).
- [x] Abandon « Resign » avec accord de l'adversaire et barème officiel (§4.11).
- [x] Tirage d'une lettre pour désigner le premier joueur (§2.1).
- [x] Feuille de match à 2 joueurs avec écart plafonné à 100 points (§5.3).

### Jeu
- [x] Annonce d'un Scrabble dans le fil (message en gras) et son du Scrabble.
- [x] Fil de jeu qui suit automatiquement le dernier message.
- [x] Glisser-déposer du chevalet symétrique gauche/droite (point de dépôt = pointeur, seuil de glissement, case survolée).
- [x] Niveaux de l'ordinateur : Débutant, Intermédiaire, Avancé, Maître (valeur des lettres gardées).
- [x] Import de liste de mots dans le dictionnaire (sauvegarde, tri, accents retirés).
- [x] Réseau : serveur modifiable, pays par défaut du système, avertissement sur l'absence de chiffrement.
- [x] Protection contre deux calculs de coups simultanés (le conseil automatique et l'enregistrement du meilleur coup
  se partageaient le moteur).

### Coach IA (optionnel)
- [x] Quatre services : Groq (par défaut), Gemini, Anthropic, OpenCode. Clé, modèle et accord séparés.
- [x] Onglet Coach : historique du plus récent au plus ancien (type, service, modèle, temps de réponse, question, réponse).
- [x] Conseil automatique à chaque tour humain (silencieux tant que l'accord n'est pas donné).
- [x] Données exactes : tuiles jouées, lettres gardées et coup recommandé calculés par le jeu, pas devinés par le modèle ;
  chevalet sans emplacement vide ; fin de partie signalée.
- [x] Conseils courts (50 mots au maximum), une nouvelle tentative sur erreur temporaire, délai explicite.
- [x] Le coach ne valide jamais un mot : le dictionnaire décide.

### Livraison
- [x] Pull request n° 4 (règlement FISF 2026) fusionnée.
- [x] Pull request n° 5 (niveaux, dictionnaire, réseau, son, coach) fusionnée.

## Limites connues
- OpenCode est intégré, mais le compte n'a ni crédit (adresse Zen) ni abonnement Go : le message du service s'affiche.
- Gemini est parfois lent ou en surcharge.
- Le coach peut encore se tromper sur des détails ; ses conseils sont indicatifs.
- Les clés d'API sont stockées sans chiffrement dans `coach.ini` (voir la tâche 11).
- Le bilan compare chaque coup au coup qui rapporte le plus de points : un coup défensif, volontairement moins
  payant, y apparaît comme une perte.
- Le bilan de partie n'existe que pour les parties jouées avec l'enregistrement actif (les parties chargées n'ont pas
  de meilleur coup enregistré).
