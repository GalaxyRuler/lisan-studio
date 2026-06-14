# خط أساس الأداء - v0.1.2-beta

تاريخ الالتقاط: 2026-05-24T19:51:15.4159244+03:00
HEAD SHA: 8cfa01e05d5d1d13d747ce9d4dc5b441e1b61552
بيئة المرجع: جهاز تطوير Windows محلي

## المنهجية

كل metric محكوم ب assertion من نوع `QVERIFY2(... budget ...)` داخل `tests/TestEditorTorture.cpp`. الشريحة التي أنتجت هذا الخط الأساسي أضافت إخراج `qInfo` قابل للقراءة لنفس قيمة الزمن مباشرة قبل فحص الميزانية. لإعادة الالتقاط:

1. شغل `.\scripts\validate.ps1`.
2. من مخرجات `acs_editor_torture_tests` استخرج الأسطر المطابقة ل `PERF metric=NAME elapsed=NUM budget=NUM`.
3. كل قيمة elapsed هي الزمن الفعلي measured wall-clock لذلك المسار.

القيم تعتمد على العتاد وستختلف بين runners. أعمدة `budget` هي الحدود العليا التي يفرضها الاختبار؛ إذا تجاوز التقاط مستقبلي الميزانية يفشل الاختبار، ويجب إصلاح الشريحة التي سببت التراجع قبل الدمج.

في تشغيلات Windows المحلية قد يحتاج Qt logging إلى إجبار stderr حتى تظهر رسائل `qInfo` في مخرجات CTest:

```powershell
$env:QT_FORCE_STDERR_LOGGING = "1"
.\scripts\validate.ps1
Select-String -Pattern "PERF metric=([^ ]+) elapsed=([0-9]+) budget=([0-9]+)" -Path build\Testing\Temporary\LastTest.log
```

## اللقطة

| Metric | Elapsed (ms) | Budget (ms) | Headroom |
|---|---:|---:|---:|
| large_file_open | 312 | 2000 | 84% |
| undo_redo_storm_100k | 66 | 500 | 87% |
| find_replace_storm_100k | 673 | 1000 | 33% |
| arabic_50k_line_render | 106 | 5000 | 98% |
| alternating_undo_redo_storm | 84 | 500 | 83% |
| find_replace_storm_300 | 46 | 1000 | 95% |
| indent_guide_paint_10k | 31 | 5000 | 99% |

Headroom = (budget - elapsed) / budget، مقربة إلى نسبة مئوية كاملة.

## متى يعاد الالتقاط؟

- قبل أي شريحة V2 تمس قلب المحرر: المؤشرات المتعددة، التحديد العمودي، تبديل syntax engine، وغيرها.
- عند ترقية الإصدار الرئيسي من Qt.
- عند تغير عتاد runner المرجعي.
- كجزء من كل حزمة أدلة beta release في المستقبل.
