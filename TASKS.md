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
- [ ] 4. Défense au niveau Maître : éviter d'offrir des cases « mot compte triple » à l'adversaire.
- [x] **5. Messages en français** : tous les textes ajoutés au jeu (99) ont une traduction française intégrée, qui
  complète la traduction téléchargée sans la remplacer (`frenchsupplement`).

### Règlement FISF 2026 (restant)
- [ ] 6. Temps épuisé (§5.1.3) : le joueur sans temps passe automatiquement mais peut encore contester ; son adversaire ne
  peut plus changer ses lettres.
- [ ] 7. Barème de sanctions (§6.4) et export de la feuille de match (PDF ou CSV).
- [ ] 8. Déclaration du joker avec la lettre représentée, notée sur la feuille de match (§3.1.4).

### Coach et dictionnaire
- [ ] 9. Onglet Coach dans la mise en page mobile.
- [ ] 10. Vérification de mise à jour du dictionnaire (liste officielle plus récente).
- [ ] 11. Clés d'API chiffrées (gestionnaire d'identifiants de Windows) au lieu de `coach.ini` en clair.

### Réseau et technique
- [ ] 12. Connexion chiffrée au serveur ; abandon et tirage au sort en partie réseau.
- [ ] 13. Installateur Windows (`windeployqt`) pour partager le jeu sans installer Qt.
- [ ] 14. Corriger l'erreur QML `ScrBoard.qml:75` (couleurs non chargées à la construction du plateau), présente avant
  tout ce travail.

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
- Le bilan de partie n'existe que pour les parties jouées avec l'enregistrement actif (les parties chargées n'ont pas
  de meilleur coup enregistré).
