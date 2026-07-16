# Preuves AI-01 — MCP et centre d’automatisation

Validation réalisée le 16 juillet 2026 sous Windows 11 avec le build Qt 5
UCRT64 de la branche `codex/ai-mcp-foundation`.

## Capture

| ID | État | Santé | Fichier |
|---|---|---|---|
| AI01-01 | Centre de démarrage, panneau **Automatisation et IA** ouvert, lecture seule, commande/configuration et quatre outils visibles | Sain à 1920×1080 | [`01-automation-center.png`](01-automation-center.png) |
| AI01-02 | Même parcours depuis le paquet Windows portable, sans MSYS2 dans le `PATH` | Sain, QML rendu depuis les modules embarqués | [`02-portable-runtime.png`](02-portable-runtime.png) |

Les captures proviennent de l’application complète compilée, pas d’une maquette
ni d’un banc réimplémentant l’écran. AI01-02 est issue du paquet portable final
contenant `qt.conf`, les greffons QML cœur et la plateforme Qt hors écran.

## Tests automatisés du lot

| Test | Couverture |
|---|---|
| `mcp_stdio_smoke_test` | Enveloppes invalides, limite 1 Mio, initialisation, outils, création sans écrasement, métadonnées, versions incompatibles, inspection, cartouches, validation, ping et arrêt |
| `automation_center_qml_test` | Chargement, texte littéral, formes plurielles Qt, compteurs, commande, activation explicite de l’écriture, défilement/focus clavier, curseur borné jusque dans un viewport de 20 px et largeur minimale |
| `automation_center_qml_large_text_150_test` | Même composant avec mise à l’échelle 150 % |

Commande de rejeu :

```powershell
ctest --test-dir C:\chemin\build `
  -R "^(mcp_stdio_smoke_test|automation_center_qml_test|automation_center_qml_large_text_150_test)$" `
  --output-on-failure
```

Les trois tests ciblés réussissent. La suite complète réussit également :
**51/51 tests CTest**, puis **17/17** contrats Windows natifs et **25/25**
contrats Windows natifs à 150 %. Le même cycle MCP passe depuis le paquet
portable avec un `PATH` limité aux composants Windows.
