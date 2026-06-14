# التحقق من Lisan Studio

يشرح هذا الملف بوابات التحقق لبناءات Windows المثبتة. يرفق مع حزم الإصدار حتى يرى المستخدمون والمراجعون ما يفترض أن يثبته كل إصدار.

## بوابة المصدر المحلي

شغل من جذر المستودع:

```powershell
.\scripts\validate.ps1
```

يهيئ هذا الأمر بناء Release، ويبني التطبيق والاختبارات، ثم يشغل CTest مع Qt في وضع offscreen.

## بوابة التغليف

ابن MSI مرشحا للإصدار:

```powershell
.\scripts\package.ps1 `
  -ProductVersion 1.0.0 `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>"
```

تتحقق بوابة التغليف من:

- وجود `LisanStudio.exe` داخل stage.
- نشر ملفات Qt runtime.
- تضمين Python runtime داخل `runtime\python`.
- نسخ `lughat-althuban` و `debugpy` إلى runtime المجهز.
- غياب مؤشرات runtime محلية/قابلة للتحرير.
- وجود ملفات التراخيص.
- كتابة MSI ودليل حالة التوقيع داخل `artifacts\`.

## بوابة التطبيق المثبت

شغلها فقط في بيئة Windows QA معزولة أو على جهاز تقصد تثبيت Lisan Studio وإزالته منه:

```powershell
.\scripts\installed-smoke.ps1
```

يتحقق سكربت installed smoke من:

- وجود الملف التنفيذي المثبت
- أن الملف التنفيذي هو `LisanStudio.exe` داخل `%LOCALAPPDATA%\LisanStudio`
- وجود Python runtime المدمج
- قدرة `lughat-althuban` على تشغيل ملف `.apy` مختلط عربي/إنجليزي
- فك ترميز المخرجات العربية ك UTF-8
- قدرة التطبيق على الانطلاق مع مسار مشروع
- قدرة التطبيق على الانطلاق مع مسار ملف
- عدم وجود مؤشرات مصدر محلي قابل للتحرير داخل runtime المدمج

## بوابة MSI

شغلها فقط في بيئة Windows QA معزولة أو على جهاز تقصد أن يحدث عليه تغيير MSI:

```powershell
.\scripts\msi-smoke.ps1
```

يتحقق سكربت MSI smoke من:

- تثبيت MSI صامت
- وجود الحمولة المثبتة داخل `%LOCALAPPDATA%\LisanStudio`
- اختصارات Start Menu و Desktop
- تضمين README، وملاحظات الإصدار، وملاحظات التحقق، وملفات التراخيص
- ملفات تراخيص Qt و Python و `lughat-althuban`
- فحص runtime المثبت عبر `scripts\installed-smoke.ps1`
- إلغاء تثبيت MSI صامت يزيل الملف التنفيذي والاختصارات

فحص الترقية:

```powershell
.\scripts\msi-upgrade-smoke.ps1 `
  -EarlierMsiPath "<old-msi>" `
  -ReplacementMsiPath "<new-msi>" `
  -ExpectedEarlierVersion "<old-version>" `
  -ExpectedReplacementVersion "<new-version>" `
  -AllowMutation `
  -IUnderstandThisRunsMsiUpgrade
```

## بوابة GitHub Actions

يستطيع المشرفون تشغيل بوابة MSI الكاملة عبر Windows runner ذاتي الاستضافة:

```powershell
gh workflow run msi-tests.yml -f scenario=full
```

يوجد workflow في `.github\workflows\msi-tests.yml`، ويرفع أدلة MSI والتثبيت والترقية ك artifacts.

## بوابة QA اليدوية

استخدم التطبيق المثبت لهذه الفحوصات:

- التشغيل من Start Menu
- التشغيل من اختصار Desktop
- فتح `samples\torture-project`
- التحقق أن shell، وشريط المشروع الجانبي، وتبويبات المحرر، واللوحة السفلية، وشريط الحالة تعمل RTL
- فتح ملف عربي/إنجليزي مختلط، وتعديله، وحفظه، وإغلاقه، وإعادة فتحه
- التحقق من حركة المؤشر عبر معرفات عربية، وأسماء إنجليزية، وأرقام، وعوامل، ومسارات Windows
- التحقق من التحديد، والنسخ، واللصق، والتراجع، والإعادة، و Backspace، و Delete قرب النص العربي
- البحث في المشروع وفتح نتيجة من تبويب نتائج البحث
- إدخال محرف BiDi مخفي في ملف مؤقت والتأكد أن لوحة Problems تبلغ عنه
- تشغيل ملف `.apy` الحالي والتأكد أن stdout/stderr، ورمز الخروج، والوقت المنقضي، وسلوك الإلغاء مقروءة
- فتح Settings، والتحقق من تشخيصات runtime، وتغيير خط المحرر، وإعادة فتح التطبيق
- إلغاء التثبيت والتأكد أن ملفات التطبيق والاختصارات أزيلت
- إعادة التثبيت دون إعداد PATH أو Python يدويا

أنشئ حزمة QA يدوية من أدلة موجودة:

```powershell
.\scripts\beta-manual-check.ps1 -ReleaseLabel "1.0.0-beta"
```

## موانع الإصدار

- تلف المؤشر أو التحديد
- ظهور المخرجات العربية بحروف تالفة
- فقدان بيانات عند الحفظ أو الفتح
- احتياج المثبت إلى PATH أو إعداد Python من النظام
- فشل فحص تثبيت/إلغاء MSI
- غياب ملفات تراخيص Qt أو Python أو `lughat-althuban`
- إدخال المحرر لمحارف BiDi مخفية
- ظهور واجهة placeholder
- تراجع RTL في شريط الأوامر العلوي، أو المشروع/الشريط الجانبي، أو التبويبات، أو شريط الحالة، أو اللوحة السفلية
- انهيار التطبيق عند الفتح أو الحفظ أو التشغيل أو الانطلاق أو الإغلاق
