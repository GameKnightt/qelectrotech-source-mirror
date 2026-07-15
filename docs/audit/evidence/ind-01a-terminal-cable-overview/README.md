# Preuves IND-01A — Borniers et câbles

## 01 — Vue peuplée déterministe

![Vue consolidée Borniers et câbles](01-overview-populated.png)

Cette image est produite par `TerminalStripOverviewTest::rendersEvidence()` avec
le widget et le modèle compilés de la préversion, sous la plateforme Windows
native. Le jeu de données détaché contient six bornes : `X2`, `X10`, deux
étages, deux bornes indépendantes, plusieurs conducteurs et un couple
câble/âme incomplet.

La capture valide le rendu, la densité, le tri métier initial, les états et la
lisibilité. Elle ne prouve pas le chargement depuis un fichier `.qet`. Ce
raccordement est contrôlé séparément dans l’application complète avec
`ArduinoLCD.qet`, qui expose correctement l’état réel « aucune borne ».
