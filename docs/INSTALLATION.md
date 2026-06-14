# تثبيت وإعداد Lisan Studio

تغطي هذه الصفحة التثبيت العام، وإعداد بيئة التطوير المحلية، والبناء من المصدر، والتغليف، والتحقق، وحل المشكلات.

## التثبيت من GitHub Release

1. افتح <https://github.com/GalaxyRuler/lisan-studio/releases>.
2. اختر الإصدار الذي تريد تثبيته.
3. حمل ملف MSI، مثل `LisanStudio-1.0.0-beta.msi`.
4. شغل MSI واتبع خطوات Windows Installer.
5. افتح **Lisan Studio** من قائمة Start أو من اختصار سطح المكتب.

إذا لم يكن الإصدار يحتوي على ملف MSI، فهذا الإصدار متاح من المصدر فقط. ابن التطبيق من المصدر أو انتظر حتى يرفق المشرف مثبتا تم التحقق منه.

يثبت MSI التطبيق لكل مستخدم داخل:

```text
%LOCALAPPDATA%\LisanStudio
```

الملف التنفيذي المتوقع بعد التثبيت:

```text
%LOCALAPPDATA%\LisanStudio\LisanStudio.exe
```

## تحذير SmartScreen عند التشغيل الأول

مثبتات Lisan Studio غير موقعة إلا إذا ذكر الإصدار عكس ذلك صراحة. عند التشغيل الأول، قد يعرض Windows مربع SmartScreen يقول إن Microsoft Defender SmartScreen منع تشغيل تطبيق غير معروف.

للمتابعة مع بناء غير موقع:

1. اضغط **More info**.
2. اضغط **Run anyway**.

قد تمنع أجهزة Windows المدارة مؤسسيا التطبيقات غير الموقعة بالكامل. راجع [ADR-0010](adr/0010-msi-code-signing.md) لقرار توقيع الكود الحالي ومحفزات إعادة مراجعته.

## إلغاء التثبيت

استخدم إعدادات Windows:

1. افتح **Settings**.
2. اذهب إلى **Apps > Installed apps**.
3. ابحث عن **Lisan Studio**.
4. اختر **Uninstall**.

يجب أن يزيل MSI ملفات التطبيق والاختصارات. إعدادات المستخدم ليست جزءا من حمولة MSI.

## الترقية من إصدارات ArabicCodeStudioQt القديمة

إذا ثبت إصدارا مبكرا باسم `ArabicCodeStudioQt` أو `Arabic Code Studio Qt`، فألغ تثبيته يدويا قبل تثبيت Lisan Studio. تلك الحزم استخدمت هوية منتج مختلفة، لذلك لا يتعامل معها Windows Installer كتطبيق واحد.

## متطلبات التطوير

البناء المحلي الافتراضي يتوقع جهاز Windows يحتوي على:

- Windows 10 أو Windows 11
- PowerShell 5.1 أو PowerShell 7
- Git مع دعم submodules
- MSYS2 UCRT64
- CMake 3.24 أو أحدث
- Ninja
- GCC من MSYS2 UCRT64
- وحدات Qt 6: Widgets و Gui و Core و Test و Concurrent

ثبت حزم MSYS2 الشائعة من MSYS2 UCRT64 shell:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base
```

إذا طلب `pacman -Syu` إغلاق shell وإعادة فتحها، افعل ذلك أولا ثم شغل أمر تثبيت الحزم مرة أخرى.

تفترض سكربتات البناء هذه المسارات افتراضيا:

```text
C:\msys64\usr\bin\bash.exe
C:\msys64\ucrt64\bin
```

إذا كان MSYS2 مثبتا في مكان آخر، مرر مسار Bash:

```powershell
.\scripts\build.ps1 -BashPath "D:\msys64\usr\bin\bash.exe"
```

## البناء

من جذر المستودع:

```powershell
git submodule update --init --recursive
.\scripts\build.ps1
```

ملف التطبيق الناتج:

```text
build\LisanStudio.exe
```

## الاختبار

شغل بوابة التحقق المعتادة:

```powershell
.\scripts\validate.ps1
```

يبني هذا الأمر التطبيق ويشغل حزمة CTest في وضع Qt offscreen.

## متطلبات التغليف

يحتاج التغليف إلى المتطلبات التالية فوق متطلبات البناء العادية:

- WiX Toolset v7 مع `wix.exe`
- مجلد Python 3.13 Runtime ليتم تضمينه
- نسخة مصدر محلية من `lughat-althuban` مع package metadata مولدة
- ملفات تراخيص Qt من حزمة MSYS2 Qt

يقبل سكربت التغليف مسارات صريحة:

```powershell
.\scripts\package.ps1 `
  -ProductVersion 1.0.0 `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>" `
  -WixPath "<path-to-wix.exe>"
```

أو استخدم متغيرات البيئة:

```powershell
$env:LISAN_APYTHON_ROOT = "<path-to-lughat-althuban>"
$env:LISAN_PYTHON_ROOT = "<path-to-python-3.13-runtime>"
```

المخرجات:

```text
artifacts\LisanStudio-<version>-beta.msi
stage\LisanStudio
```

لتهيئة مجلد التطبيق دون إنشاء MSI:

```powershell
.\scripts\package.ps1 -SkipMsi
```

## فحص التطبيق المثبت

بعد تثبيت MSI في بيئة Windows QA معزولة، تحقق من التطبيق المثبت:

```powershell
.\scripts\installed-smoke.ps1
```

يتحقق السكربت من وجود الملف التنفيذي، و Python runtime المدمج، وتشغيل ملفات `.apy` العربية، والتقاط مخرجات UTF-8، وتشغيل التطبيق بمسار مشروع أو ملف، وشكل حزمة runtime.

## فحص MSI

فحص تثبيت MSI وإلغاء تثبيته وترقيته يغير الجهاز. لا تشغله على جهاز عمل نشط إلا إذا كنت تقصد تثبيت التطبيق أو إزالته هناك.

فحص التثبيت/الإلغاء:

```powershell
.\scripts\msi-smoke.ps1
```

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

Workflow على GitHub Actions:

```powershell
gh workflow run msi-tests.yml -f scenario=install
gh workflow run msi-tests.yml -f scenario=upgrade
gh workflow run msi-tests.yml -f scenario=full
```

يوجد workflow في `.github\workflows\msi-tests.yml`، ويتوقع runner ذاتي الاستضافة على Windows يحتوي على Qt و WiX و Python وأدوات التغليف.

## أدلة الإصدار

أنشئ حزمة أدلة الإصدار في بيئة Windows QA معزولة:

```powershell
.\scripts\release-evidence.ps1 `
  -ProductVersion 1.0.0 `
  -ReleaseLabel "1.0.0-beta" `
  -ApythonRoot "<path-to-lughat-althuban>" `
  -PythonRoot "<path-to-python-3.13-runtime>"
```

الأدلة المتوقعة:

```text
artifacts\release\<release-label>\VALIDATION_LOG.md
artifacts\release\<release-label>\CHECKSUMS-SHA256.txt
artifacts\release\<release-label>\KNOWN_ISSUES.md
artifacts\release\<release-label>\screenshots\main-window.png
```

## حزمة QA اليدوية

أنشئ حزمة QA يدوية من أدلة موجودة:

```powershell
.\scripts\beta-manual-check.ps1 -ReleaseLabel "1.0.0-beta"
```

ينتج السكربت:

```text
artifacts\beta-manual-check\<run-id>\manual-beta-qa.docx
artifacts\beta-manual-check\<run-id>\manual-beta-qa.md
artifacts\beta-manual-check\<run-id>\manual-beta-qa.json
```

ملف Word هو قائمة الفحص الموجهة للمراجع. ملفات Markdown و JSON هي أدلة تتبع.

## حل المشكلات

### `C:\msys64\usr\bin\bash.exe` غير موجود

ثبت MSYS2 أو مرر مسار Bash الصحيح:

```powershell
.\scripts\build.ps1 -BashPath "D:\msys64\usr\bin\bash.exe"
```

### CMake لا يجد Qt 6

تأكد أن حزمة UCRT64 Qt مثبتة وأن سكربت البناء يستخدم مسار UCRT64 أولا:

```text
/ucrt64/bin
```

### `windeployqt6.exe` غير موجود

تأكد أن أدوات Qt موجودة في:

```text
C:\msys64\ucrt64\bin\windeployqt6.exe
```

أو مرر:

```powershell
.\scripts\package.ps1 -WindeployQtPath "D:\msys64\ucrt64\bin\windeployqt6.exe"
```

### `wix.exe` غير موجود

ثبت WiX Toolset v7 أو مرر المسار:

```powershell
.\scripts\package.ps1 -WixPath "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
```

### التطبيق المثبت يطلب Python من النظام

عامل هذا كفشل تغليف. يجب أن يستخدم MSI runtime المدمج داخل:

```text
%LOCALAPPDATA%\LisanStudio\runtime\python
```

### مخرجات العربية تظهر بحروف تالفة

عامل هذا كمانع إصدار. يجب أن يلتقط `scripts\installed-smoke.ps1` المخرجات العربية ويفك ترميزها ك UTF-8.
