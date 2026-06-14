# 0010. توقيع MSI

## الحالة

مقبول (2026-05-24) - تأجيل التوقيع لمرحلة V1.5، وإعادة التقييم عند نمو الجمهور أو ظهور احتياج من مختبري المؤسسات.

## السياق

إصدارات v0.1.1-beta شحنت ملفات MSI غير موقعة. يسجل `artifacts/SIGNING_STATUS.txt` القيم `AuthenticodeStatus=NotSigned` و `Signer=None`. ينتج هذا الملف من `Write-SigningStatus` داخل `scripts/package.ps1`.

الأثر الظاهر للمستخدم بدون توقيع: يعرض Windows SmartScreen رسالة "Windows protected your PC. Microsoft Defender SmartScreen prevented an unrecognized app from starting" عند التشغيل الأول لملف MSI غير موقع تم تنزيله. يحتاج المستخدم إلى الضغط على "More info" ثم "Run anyway" للمتابعة. هذا مألوف لمن يستخدم برامج beta غير موقعة، لكنه احتكاك حقيقي للمستخدم الجديد، وقد يمنع التطبيق بالكامل في بعض بيئات المؤسسات المدارة.

تكلفة التوقيع:

- شهادة Authenticode OV (Organization Validated): تقريبا 200-500 دولار سنويا. قد يبقى التحذير ظاهرا في البداية إلى أن يبني الملف سمعة عبر تثبيتات المستخدمين.
- شهادة Authenticode EV (Extended Validation): تقريبا 300-700 دولار سنويا. غالبا لا يظهر تحذير SmartScreen من أول تثبيت. تحتاج إلى hardware token مثل HSM أو smart card لحفظ المفتاح.
- دمج CI: خطوة `signtool.exe` بعد `package.ps1`، أو خطوة GHA بعد build job. تحتاج إلى تخزين آمن للمفتاح مثل Azure Key Vault، أو GitHub secret مشفر يحتوي PFX، أو HSM.

## القرار

تأجيل توقيع الكود لمرحلة V1.5. الاستمرار في شحن MSI غير موقع، مع توثيق تجربة SmartScreen بوضوح في مستندات المستخدم حتى يعرف مختبرو beta ما يتوقعونه عند أول تثبيت.

## السبب

- جمهور التوزيع في V1.5 خاص/early beta؛ تكلفة الاحتكاك للمستخدم الجديد محدودة ومقبولة لهذا الجمهور.
- التكلفة، وتشمل تجديد الشهادة سنويا وعمل دمج CI، أكبر من الفائدة لحجم الجمهور الحالي.
- الحصول على شهادة EV يتضمن تأخير شحن hardware token غالبا لمدة 1-2 أسبوع، وهذا يضيف تأخيرا تقويميا لأي قرار توقيع مستقبلي.
- بعد شحن بناءات موقعة، الرجوع إلى غير موقعة سيكون خطوة إلى الخلف؛ لذلك نؤجل حتى نكون مستعدين للالتزام بالتكلفة الدائمة.

## محفزات إعادة المراجعة

أعد تقييم القرار عند تحقق أي مما يلي:

- نمو جمهور التوزيع إلى أكثر من حوالي 50 مستخدما نشطا.
- إبلاغ مختبر بأن SmartScreen منعه في بيئة مؤسسة مدارة، مثل Defender SmartScreen Block enforcement أو AppLocker أو WDAC.
- إعلان الإتاحة العامة ل Lisan Studio.
- تضمين مرحلة V2 أو ما بعدها معيار قبول يقول صراحة إن المنتج شحن لمستخدمين غير beta.

## البدائل التي تمت دراستها

1. **الشحن الآن بتوقيع EV Authenticode.** رفض: نسبة التكلفة إلى الجمهور غير مناسبة، وتأخير الحصول على token، وتوقيع beta سريع التغير يزيد مخاطر توقيع artifacts دون فائدة متناسبة.
2. **الشحن الآن بتوقيع OV Authenticode.** رفض: فترة بناء سمعة SmartScreen تعني أن المستخدمين الأوائل قد يرون التحذير أيضا؛ الكلفة أقل من EV لكن مشكلة الجمهور نفسها باقية.
3. **شهادة self-signed.** رفض: SmartScreen يعامل self-signed أحيانا أسوأ من غير الموقع لأنه يعرض تحذير ناشر غير موثوق. لا توجد فائدة حقيقية لهذا نموذج التوزيع.

## توثيق موجه للمستخدم

يجب أن تذكر فقرة "المشكلات المعروفة" في ملاحظات كل إصدار تحذير SmartScreen بوضوح، مع مسار النقر الدقيق: عندما يعرض Windows الرسالة "Microsoft Defender SmartScreen prevented an unrecognized app from starting"، اضغط "More info"، ثم "Run anyway".

تم تنفيذ المتابعة: أضيفت فقرة "تحذير SmartScreen عند التشغيل الأول" إلى `docs/INSTALLATION.md`، وأضيفت نقطة SmartScreen إلى `docs/KNOWN_ISSUES.md`. كلاهما يشير إلى هذا ADR.

## النتائج

- سيستمر مختبرو beta في رؤية SmartScreen عند أول تثبيت. هذا مقبول للجمهور الحالي.
- ستستمر حزم أدلة الإصدار في تسجيل `AuthenticodeStatus=NotSigned`. يجب ألا يعتبر المراجعون ذلك عيبا؛ فهو مقصود وفق هذا ADR.
- يحتوي `scripts/package.ps1` على hook اختياري لتوقيع Authenticode. يعمل التوقيع فقط عند تمرير `-SigningCertificateThumbprint` أو ضبط `LISAN_SIGNING_CERT_THUMBPRINT`، باستخدام `signtool.exe`، و SHA-256 file digesting، و RFC3161 timestamping. قرار تخزين مفتاح CI ما زال معلقا؛ workflow يربط فقط مراجع secret/variable ولا يخزن مواد الشهادة داخل المستودع.
- السطر الموجود في `SIGNING_STATUS.txt`: `Policy: Public distribution requires a valid Authenticode signature` يبقى صحيحا كهدف عام؛ أما مؤهل "private beta" الحالي في محتوى الملف فهو ما ينطبق خلال V1.5.
