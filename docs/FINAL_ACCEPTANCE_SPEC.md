# new_mavlink — مواصفة القبول النهائية

## 1. تعريف المنتج

`new_mavlink` هو بروتوكول اتصالات أصلي للطائرة وGCS والـproxy. اسمه التجاري لا يعني أنه MAVLink؛ الـwire والرسائل والـsession والـQoS كلها ملك لـnew_mavlink. MAVLink 2 ليس أساس النقل، ولا يدخل native core. يوجد adapter اختياري منفصل فقط لتوصيل PX4 أو GCS التي لا تتكلم new_mavlink native.

## 2. المسار الإلزامي

يجب أن يعمل المسار الأساسي التالي دون MAVLink:

```text
Native Vehicle <— encrypted new_mavlink —> Native Proxy <— encrypted new_mavlink —> Native GCS
```

ويجب أن يثبت المسار command وACK وstate/event telemetry والاشتراك وإلغاء الاشتراك وsnapshot وdelta وresync. proxy ليس UDP relay أعمى؛ هو message-aware ويطبق authorization وQoS وbackpressure وsession isolation.

## 3. كفاءة الموارد

لا يوجد بث دوري عام لكل الرسائل. كل state stream يحتاج subscription صريحة. الرسالة الأولى snapshot، والتغييرات اللاحقة delta مرتبطة بـgeneration/base_generation. عند فقد delta أو اختلاف base يرسل المستقبل ResyncRequest ويعيد المصدر snapshot. state uses latest-wins، بينما command وcritical use bounded reliable queues. توجد حدود ثابتة للذاكرة والحجم وعدد الاشتراكات والمصادر.

Heartbeat/liveness فقط دوري بميزانية محددة. critical events ترسل فورًا مع rate limit وcoalescing عند الضرورة. الرسائل التي لا تتغير لا تعاد فقط بسبب timestamp؛ ويمكن للمستقبل طلب snapshot صراحة.

## 4. الأمن

يستخدم البروتوكول NIST Ascon-AEAD128، والـnonce لا يعاد استخدامه داخل session/epoch/direction. كل record محمي بـAAD من header canonical bytes، ويرفض wrong key وAAD/payload/tag tamper وreplay وexpired record وsequence exhaustion. session establishment موثق ومربوط بالـtranscript، والمفاتيح اتجاهية مع epoch وrekey boundary.

لا يوجد plaintext fallback. مفاتيح الاختبار لا تسمى production provisioning. لا توجد covert commands أو noise-to-command؛ traffic shaping المعلن، إن أضيف، لا يدخل command decoder.

## 5. typed message families

يجب أن تكون الرسائل typed ومحددة الطول والحدود، وتشمل على الأقل: session control، schema/capability، subscribe/unsubscribe، snapshot/delta/resync، heartbeat، vehicle state، position، attitude، battery، link statistics، safety/failsafe/geofence events، command request/ack/result، rate report، وdiagnostic events.

## 6. boundary adapter

أي تحويل إلى MAVLink 2 يكون typed ومعلنًا ومحصورًا في adapter: native CommandRequest إلى allowlisted MAVLink command، وMAVLink ACK/state إلى native typed messages. لا raw MAVLink tunnel، ولا يسمح adapter بتمرير command غير موجود في allowlist أو خارج deadline/policy.

## 7. معيار الاختبار قبل التسليم

لا يسمى الإصدار final إلا بعد نجاح: native vehicle/proxy/GCS process E2E؛ 100% unit/integration tests؛ Debug وRelease؛ ASan/UBSan؛ fuzz bounded parser؛ wrong-key/tamper/replay؛ loss/duplicate/reorder؛ delta base mismatch وresync؛ queue pressure؛ subscription/resource limits؛ session rekey boundary؛ وPX4 SITL/QGC regression من الحزمة نفسها عبر adapter.

تُسجل كل نتيجة بوضوح إلى `PASS` أو `FAIL` أو `NOT RUN`. fake-PX4 لا يثبت PX4 الحقيقي، وظهور Vehicle 1 في اختبار المشروع القديم لا يثبت new_mavlink.

## 8. معنى “أفضل” في هذا المشروع

الأفضل هنا ليس ادعاء انعدام الأخطاء، بل تصميم صغير قابل للتدقيق، bounded، fail-closed، موفر للوصلة، ومثبت باختبارات قابلة لإعادة التشغيل. أي جزء غير منفذ أو غير مختبر يبقى معلنًا ولا يدخل في وصف الإصدار النهائي.
