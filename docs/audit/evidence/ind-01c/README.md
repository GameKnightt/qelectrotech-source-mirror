# Preuves IND-01C — Édition exacte des conducteurs

Captures natives de la build Windows Qt 5 exécutée le 15 juillet 2026 sur une
copie locale du projet public `industrial.qet` (50 folios).

1. [`01-cable-catalog-c20-selected.png`](01-cable-catalog-c20-selected.png) —
   catalogue chargé, câble `C20` sélectionné, action d’édition disponible ;
2. [`02-exact-conductor-editor.png`](02-exact-conductor-editor.png) — brouillon
   limité aux deux conducteurs de `C20`, avec uniquement **Couleur** et
   **Câble** modifiables.

Le parcours a aussi été ouvert par `Alt+M`, puis annulé. La copie du projet est
restée inchangée. Les modifications et l’Undo atomique sont couverts par les
tests `conductor_bulk_edit_exact_selection_test` et
`conductor_change_preview_test`.
