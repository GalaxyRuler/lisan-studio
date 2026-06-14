# خارطة طريق Lisan Studio V3

> الحالة: اكتملت عند `v1.0.0-rc1`.
> بنيت الخطة الأصلية في 2026-06-05 على HEAD `7185cd1` بعد إغلاق V2 عند `v0.5.0-beta`.
> أرشيف V2 موجود في [docs/ROADMAP-V2.md](ROADMAP-V2.md).
> إيقاع الإصدار: لكل phase إصدار. أغلقت V3 عند `v1.0.0-rc1`.

## المهمة

تضيف V3 واجهة Git مدمجة ودعم مساحات العمل متعددة الجذور. يصبح Lisan Studio بيئة تطوير عربية أولا ومكتفية بذاتها في المهام اليومية، دون الحاجة إلى Git client خارجي.

## النطاق

**V3 تشمل:**

- لوحة حالة Git، وعارض diff، و stage/unstage للملفات أو hunks.
- تدفق commit: رسالة، commit، push، pull، fetch.
- إدارة الفروع: إنشاء، تبديل، دمج، حذف.
- لوحة سجل history و blame gutter داخل EditorSurface.
- مساحات عمل متعددة الجذور للمونوريبو أو المشاريع متعددة الوحدات.

**V3 لا تشمل:**

- محرر دمج ثلاثي تفاعلي (V4).
- واجهة مراجعة PR من GitHub/GitLab (V4).
- دعم Git LFS (V4).
- نظام extensions/plugins (V4).
- نقل التطبيق لمنصات أخرى (غير مخطط).
- ARM64 Windows (غير مخطط).

## قرار البنية

**ADR-0016** (`docs/adr/0016-git-backend.md`): استخدام libgit2 عبر CMake FetchContent. fallback هو git plumbing subprocess. أوامر porcelain ليست fallback.

## بنية المراحل

```text
G1 ADR-0016 (docs-only, gates G2+)
   └──> G2 Repository state model (GitRepository class, decorations, status bar)
           ├──> G3 Status panel + diff viewer --> v0.6.0-beta
           │       └──> G4 Commit workflow (stage, message, push/pull) --> v0.7.0-beta
           │               └──> G6 History + blame gutter --> v0.8.0-beta --> v1.0.0-rc1
           └──> G5 Branch management --> v0.7.0-beta (parallel with G3 to G4)
MR Multi-root workspaces (independent) --> v0.9.0-beta --> v1.0.0-rc1
```

## قائمة الشرائح

| الشريحة | العنوان | تعتمد على | الهدف |
|---|---|---|---|
| G1 | ADR-0016 Git backend | لا شيء | docs-only |
| G2-a | نموذج حالة GitRepository | G1 | - |
| G2-b | زخارف dirty في شجرة المشروع | G2-a | - |
| G2-c | مؤشر الفرع و dirty في شريط الحالة | G2-a | - |
| G3-a | لوحة حالة Git | G2-a | - |
| G3-b | عارض diff مدمج | G3-a | - |
| G3-c | قطع إصدار v0.6.0-beta | G3-b | v0.6.0-beta |
| G4-a | stage / unstage hunks | G3-b | - |
| G4-b | لوحة commit وفعل commit | G4-a | - |
| G4-c | Push / Pull / Fetch | G4-b | - |
| G4-d | قطع إصدار v0.7.0-beta | G4-c + G5-b | v0.7.0-beta |
| G5-a | منتقي الفروع: switch + create | G2-a | - |
| G5-b | merge + delete branch | G5-a | - |
| G6-a | لوحة history log | G4-b | - |
| G6-b | blame gutter داخل EditorSurface | G2-a | - |
| G6-c | diff عند commit تاريخي | G6-a + G6-b | - |
| G6-d | قطع إصدار v0.8.0-beta | G6-c | v0.8.0-beta |
| MR-a | نموذج مساحة عمل متعددة الجذور | لا شيء | - |
| MR-b | واجهة فتح/إزالة root إضافي | MR-a | - |
| MR-c | قطع إصدار v0.9.0-beta | MR-b | v0.9.0-beta |
| RC | إغلاق مرحلة v1.0.0-rc1 | G6-d + MR-c | v1.0.0-rc1 |

## سجل المخاطر

| الخطر | التخفيف |
|---|---|
| ربط libgit2 عبر CMake ثقيل جدا | fallback إلى git-plumbing subprocess وفق ADR-0016 |
| تلف أسماء الملفات العربية في عمليات Git | اختبار `core.quotePath=false` و UTF-8 locale؛ plumbing أكثر موثوقية من porcelain |
| اتساع نطاق واجهة حل التعارضات | تعرض V3 markers فقط؛ الدمج الثلاثي التفاعلي في V4 |
| أداء شجرة المشروع متعددة الجذور | تحميل حالة كل root بتكاسل مع cache TTL لمدة 5 ثوان |

## معايير اكتمال V3

تغلق V3 عندما تكتمل كل الشرائح G1-RC و MR-a-MR-c أو تؤجل صراحة. إغلاق المرحلة يقطع `v1.0.0-rc1`.

---

*أنشئت الخطة الأصلية في 2026-06-05.*
