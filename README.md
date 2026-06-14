# Lisan Studio

بيئة تطوير أصلية على Windows لكتابة وتشغيل مشاريع `.apy` و"بايثون العربي" أولا.
المشروع مبني ب ++C17 و Qt 6. لا يستخدم Electron ولا غلاف متصفح ولا JVM.

**إصدار المصدر الحالي:** [v1.0.0-rc1](https://github.com/GalaxyRuler/lisan-studio/releases/tag/v1.0.0-rc1)

> ملاحظة عن المثبت العام: استخدم ملف MSI المرفق في صفحة GitHub Releases عندما يكون موجودا. إذا كان الوسم لا يحتوي على ملف MSI، فهذا يعني أن الإصدار متاح من المصدر فقط، أو أن المثبت لم يرفق بعد.

## ماذا يقدم Lisan Studio؟

- واجهة سطح مكتب Qt عربية أولا، مع تخطيط RTL مناسب للعمل اليومي.
- تحرير ملفات `.apy` و `.py` و `.md` والملفات النصية، مع أرقام أسطر، تبويبات، حفظ/إعادة تحميل، واستعادة المسودات غير المحفوظة.
- تلوين صياغة لملفات `.apy`، وتحذير من محارف Unicode BiDi المخفية، ومطابقة الأقواس، وأدلة المسافة البادئة، وإظهار المسافات، وحذف الفراغات الزائدة عند الحفظ.
- تحرير متعدد المؤشرات: `Alt+Click`، وتحديد عمودي عبر `Alt+Drag`، و `Ctrl+D`، و `Ctrl+Shift+L`، وأوامر إضافة مؤشر أعلى/أسفل.
- بحث واستبدال مع regex، والتفاف البحث، وتظليل جميع النتائج.
- شجرة مشروع، ومساحات عمل متعددة الجذور، ومشاريع حديثة، وإعدادات مساحة العمل.
- تشغيل ملفات `.apy` عبر Runtime مدمج يحتوي على Python و `lughat-althuban`.
- دعم LSP للإكمال، والشرح عند المرور، والانتقال إلى التعريف، والمراجع، وإعادة التسمية، والمخطط، ورموز مساحة العمل، والرموز الدلالية.
- دعم التصحيح عبر `debugpy`، ويشمل نقاط التوقف، والتحكم بالتشغيل، والمتغيرات، والمراقبات، ولوحة مكدس الاستدعاء.
- تبويب طرفية مدمج مع فحص ثقة المشروع قبل التنفيذ.
- واجهة Git مدمجة: الحالة، stage/unstage، diff موحد، commit، push، pull، fetch، إدارة الفروع، التاريخ، blame، والفروقات التاريخية.
- سكربتات للتشخيص، والمشكلات المعروفة، وأدلة الإصدار، وبناء MSI، وفحوصات المثبت للمشرفين.

## التثبيت

### الخيار 1: تثبيت إصدار MSI جاهز

1. افتح [GitHub Releases](https://github.com/GalaxyRuler/lisan-studio/releases).
2. حمل ملف MSI الخاص بالإصدار، مثل `LisanStudio-1.0.0-beta.msi`.
3. شغل ملف MSI واتبع خطوات Windows Installer.
4. افتح **Lisan Studio** من قائمة Start أو من اختصار سطح المكتب.

يثبت MSI التطبيق لكل مستخدم داخل:

```text
%LOCALAPPDATA%\LisanStudio
```

الملف التنفيذي المتوقع هو:

```text
%LOCALAPPDATA%\LisanStudio\LisanStudio.exe
```

مثبتات Lisan Studio غير موقعة حاليا إلا إذا ذكرت ملاحظات الإصدار عكس ذلك. قد يعرض Windows SmartScreen تحذيرا عند التشغيل الأول. راجع [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) و [ADR-0010](docs/adr/0010-msi-code-signing.md).

### الخيار 2: البناء من المصدر

استنسخ المستودع مع submodules:

```powershell
git clone --recurse-submodules https://github.com/GalaxyRuler/lisan-studio.git
cd lisan-studio
```

لنسخة موجودة مسبقا:

```powershell
git submodule update --init --recursive
```

ثبت متطلبات التطوير المذكورة في [docs/INSTALLATION.md](docs/INSTALLATION.md)، ثم ابن المشروع:

```powershell
.\scripts\build.ps1
```

سيكون الملف التنفيذي الناتج في:

```text
build\LisanStudio.exe
```

## الاستخدام الأول

1. افتح Lisan Studio.
2. افتح مجلدا يحتوي على ملفات `.apy`، أو أنشئ ملف `.apy` جديدا.
3. اكتب كود بايثون العربي داخل المحرر.
4. استخدم **Run Current File** لتشغيل ملف `.apy` الحالي.
5. استخدم اللوحة السفلية للمخرجات، والمشكلات، ونتائج البحث، والطرفية، وجلسات التصحيح.
6. استخدم لوحة Source Control لعرض حالة Git، والفروقات، و staging، و commits، والفروع، و blame، والتاريخ.

توجد ملفات عينة داخل:

```text
samples\torture-project
```

## التطوير

فحص المشروع المعتاد:

```powershell
.\scripts\validate.ps1
```

هذا السكربت يهيئ بناء Release، ويبني التطبيق والاختبارات، ثم يشغل CTest مع Qt في وضع offscreen.

لبناء MSI مخصص للتحقق من الإصدار:

```powershell
.\scripts\package.ps1 -ProductVersion 1.0.0 -ApythonRoot "<path-to-lughat-althuban>" -PythonRoot "<path-to-python-3.13-runtime>"
```

يمكن للمشرفين استخدام متغيرات البيئة بدلا من تمرير المسارات كوسائط:

```powershell
$env:LISAN_APYTHON_ROOT = "<path-to-lughat-althuban>"
$env:LISAN_PYTHON_ROOT = "<path-to-python-3.13-runtime>"
```

تفاصيل الإعداد والبناء وحل المشكلات موجودة في [docs/INSTALLATION.md](docs/INSTALLATION.md).

## التحقق وجاهزية النشر العام

تتبع فحوصات النشر العام في [docs/PUBLIC_RELEASE_READINESS.md](docs/PUBLIC_RELEASE_READINESS.md).

الفحص المحلي المستخدم قبل نشر تغييرات المصدر:

```powershell
.\scripts\validate.ps1
```

فحوصات تثبيت MSI وإلغاء تثبيته وترقيته والتقاط الصور وفحص التطبيق المثبت يجب أن تعمل في بيئة Windows QA معزولة، وليس على جهاز عمل نشط. Workflow الخاص بذلك:

```powershell
gh workflow run msi-tests.yml -f scenario=full
```

## خارطة الطريق

| المرحلة | الحالة | الإصدار |
|---|---|---|
| V1.5 - مؤشرات متعددة ومسار MSI | تم الشحن | v0.2.0-beta |
| V2 - LSP ومصحح وطرفية | تم الشحن | v0.5.0-beta |
| V3 - واجهة Git ومساحات عمل متعددة الجذور | تم الشحن | v1.0.0-rc1 |
| V4 - دمج ثلاثي، مراجعة PR، وإضافات | مخطط | - |

خارطة V3: [docs/ROADMAP-V3.md](docs/ROADMAP-V3.md). أرشيف V2 التاريخي موجود في [docs/ROADMAP-V2.md](docs/ROADMAP-V2.md).

## الأمان

لا ترفع ملفات `.env*`، أو المفاتيح الخاصة، أو الشهادات، أو tokens، أو مواد التوقيع، أو بيانات اعتماد runtime خاصة. أبلغ عن الثغرات عبر [SECURITY.md](SECURITY.md).

## المساهمة

راجع [CONTRIBUTING.md](CONTRIBUTING.md) لإعداد بيئة التطوير، وقواعد الفروع، وتوقعات التحقق، وقواعد الإصدار.
