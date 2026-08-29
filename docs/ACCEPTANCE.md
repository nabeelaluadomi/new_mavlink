# new_mavlink — معايير القبول

new_mavlink هو بروتوكول رسائل native مستقل للطائرة وGCS والـproxy. MAVLink 2 ليس wire protocol للمشروع؛ يوجد فقط adapter اختياري على حدود PX4/QGC.

يجب أن يدعم الإصدار القابل للتسليم: جلسات موثقة ومفاتيح اتجاهية مع Ascon-AEAD128؛ wire framing bounded؛ typed messages؛ subscriptions وsnapshot/delta/resync؛ QoS وlatest-wins وbackpressure؛ proxy process متعدد الأطراف؛ native vehicle وnative GCS؛ منع التكرار والرسائل المنتهية؛ resilience للفقد والتكرار وإعادة الترتيب؛ واختبارات security وfuzz وsanitizer والأداء.

لا يقبل التسليم على أنه كامل إذا كان مجرد library demo أو fake-PX4 فقط. يجب أن يمر native vehicle→proxy→native GCS أولًا، ثم PX4 SITL adapter regression من الحزمة نفسها، مع فصل النتائج بين native protocol وMAVLink compatibility.

الاتصال المقصود بـconversion هو typed conversion معلن. لا توجد قناة covert ولا أوامر داخل noise. يمكن استخدام traffic shaping وCoverSlot معلنة فقط، ولا يفسر أي cover record كأمر.
