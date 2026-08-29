# new_mavlink

`new_mavlink` بروتوكول native مستقل للطائرة والـproxy والـGCS. لا يستخدم MAVLink 2 كأساس للنقل، ولا يمرر raw MAVLink bytes داخل native records. يعتمد الـcore على wire/header ورسائل typed وsession اتجاهية وAscon-AEAD128، بينما يوجد MAVLink 2 adapter منفصل عند حدود PX4/QGC فقط.

> **حالة التحقق:** اجتازت الشجرة الحالية Debug وRelease وASan/UBSan، وكل مسارات CTest الإحدى عشرة ناجحة، بما في ذلك messaging advanced وpublisher policy وprocess E2E Vehicle→Proxy→GCS. اختبار PX4 SITL الحقيقي من هذه الشجرة يبقى بوابة منفصلة إذا لم يكن PX4 موجودًا في البيئة.

## البنية

| الجزء | الوظيفة |
|---|---|
| `include/new_mavlink/wire.hpp` | native wire contract والأنواع وQoS |
| `src/record.cpp` | canonical header وCRC وAEAD record وreplay وnonce-reuse guard |
| `src/crypto.cpp` | Ascon-AEAD128 وAscon-PRF128 الرسميان مع directional keys |
| `src/session.cpp` | HELLO/ACCEPT/FINISH transcript session |
| `src/messages.cpp` | codecs typed للـcommand/ack/subscription/resync/Position/Attitude/Battery/Heartbeat |
| `src/qos.cpp` | bounded queues وcritical priority وlatest-wins حسب resource |
| `src/proxy.cpp` | proxy message-aware مع subscriptions/cache/delta-gap resync وsession re-encryption |
| `apps/vehicle_endpoint.cpp` | native vehicle process |
| `apps/proxy_service.cpp` | proxy process ينهي ingress session ويعيد تشفير egress |
| `apps/gcs_endpoint.cpp` | native GCS process one-shot E2E |
| `include/new_mavlink/publisher.hpp` و`src/publisher.cpp` | سياسة on-change وhelpful full snapshot الدوري |
| `apps/publisher_demo.cpp` | دليل تشغيل زمني لسياسة النشر |
| `include/new_mavlink/mavlink2_adapter.hpp` | boundary adapter منفصل فقط |
| `apps/px4_adapter_probe.cpp` | probe اختياري لـPX4 SITL COMMAND_LONG/ACK |

## البناء والاختبار

من جذر المشروع:

```bash
cd /home/ubuntu/new_mavlink
rm -rf build
cmake -S . -B build -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --parallel 2
```

يجب أن تظهر `100% tests passed, 0 tests failed out of 11`. يشمل ذلك `newmavlink_publisher_test` الذي يثبت on-change وfull helpful snapshots عند 10000 و20000ms. اختبارات fuzz deterministic تشغل 100000 مدخل bounded. لا تستخدم مجلد build قديم عند مقارنة النتائج.

لبوابة ASan/UBSan:

```bash
cd /home/ubuntu/new_mavlink
rm -rf build-asan
cmake -S . -B build-asan -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Debug -DNEWMAVLINK_SANITIZERS=ON
cmake --build build-asan --parallel 2
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-asan --output-on-failure --parallel 2
```

## تشغيل native process E2E يدويًا

افتح ثلاث نوافذ طرفية من جذر المشروع. في الأولى شغّل GCS:

```bash
./build/newmavlink_gcs 24661
```

في الثانية شغّل proxy:

```bash
./build/newmavlink_proxy 24660 24661
```

في الثالثة شغّل vehicle بمصدر UDP ephemeral:

```bash
./build/newmavlink_vehicle 0 24660
```

هذا هو وضع one-shot الخاص باختبار المسار الأساسي. لتشغيل publisher لمدة 20.5 ثانية، استخدم معامل المدة الثالث في البرامج الثلاثة:

```bash
./build/newmavlink_gcs 24661 20500
./build/newmavlink_proxy 24660 24661 20500
./build/newmavlink_vehicle 0 24660 20500
```

في وضع stream تُرسل Position وAttitude وBattery فور حدوث التغير، ثم تُرسل helpful full snapshots لهذه الحالة عند 10000ms و20000ms. لا تُرسل الحالة الثابتة كل ثانية؛ heartbeat والرسائل الحرجة لهما سياسات مستقلة.

المسار المثبت هو `Vehicle native → encrypted new_mavlink → Proxy session termination/re-encryption → encrypted new_mavlink → GCS native`. كما يمكن تشغيله آليًا عبر:

```bash
ctest --test-dir build -R newmavlink_process_e2e --output-on-failure
```

المفاتيح الثابتة داخل هذه التطبيقات simulation-only للاختبار. لا تسمها provisioning إنتاجيًا ولا تستخدمها على hardware.

## MAVLink 2 / PX4 boundary

يبقى `newmavlink_core` مستقلًا عن MAVLink. target `newmavlink_mavlink2_adapter` هو التحويل typed فقط؛ يرفض command غير موجود في allowlist، ويحوّل Position وAttitude وBattery إلى هياكل native محددة. لا يعني وجود adapter أن QGroundControl أصبح native new_mavlink UI.

إذا كان PX4 SITL موجودًا، شغّله فقط من مجلد build الصحيح:

```bash
cd "$HOME/Documents/PX4-Autopilot/build/px4_sitl_default"
PX4_SIM_MODEL=sihsim_quadx ./bin/px4 -s etc/init.d-posix/rcS
```

ثم، في طرفية أخرى، شغّل probe:

```bash
cd /home/ubuntu/new_mavlink
./build/newmavlink_px4_probe 14540 14580 400
```

النجاح الحقيقي يتطلب ظهور `NEWMAVLINK_PX4_ADAPTER_ACK=PASS`. إذا لم يوجد الملف `build/px4_sitl_default/bin/px4` فلا تُنفذ الأمر السابق؛ شغّل بدلًا منه native CTest فقط وسجّل PX4 على أنه `NOT RUN`. تشغيل QGC AppImage يعرض Vehicle 1 عبر MAVLink إذا وُجد رابط MAVLink، لكنه ليس إثباتًا أن QGC يتكلم new_mavlink native.

## حدود أمنية مهمة

كل native record يستخدم canonical header كـAAD وAscon-AEAD128 tag. يتم رفض wrong key وtamper وreplay وexpired records والـsequence غير المتزايد. توجد directional keys وepoch boundary وrekey API؛ أما provisioning الإنتاجي وPKI وforward secrecy فتحتاج طبقة lifecycle إضافية ولم تُسمَّ هنا مكتملة.

لا يحتوي المشروع covert command channel ولا يخفي commands داخل noise. أي traffic shaping مستقبلي يجب أن يبقى معلنًا ومنفصلًا تمامًا عن command decoder.

## سجل التحقق

راجع `docs/VERIFICATION_PROGRESS_AR.md` و`docs/FINAL_ACCEPTANCE_SPEC.md`. لا يوصف المشروع بأنه flight-ready أو خالٍ من العيوب لمجرد نجاح اختبارات محلية؛ اختبار PX4 SITL/QGC يجب أن يُسجل من نفس archive وبنتيجة صريحة `PASS` أو `NOT RUN`.
