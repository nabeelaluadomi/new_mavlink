# سجل تحقق new_mavlink

هذا الملف يسجل النتائج الفعلية من الشجرة الحالية فقط. لا يستخدم كلمة **final** لإخفاء بوابة لم تُشغّل.

| البوابة | النتيجة | الدليل |
|---|---:|---|
| native wire/message codecs | PASS | `newmavlink_wire_test`, `newmavlink_messages_test` |
| Ascon-AEAD128 وAscon-PRF128 vendor integration | PASS | `newmavlink_security_test` وlink ناجح |
| HELLO/ACCEPT/FINISH transcript session | PASS | `newmavlink_session_test` |
| directional key seal/open | PASS | `newmavlink_session_test` |
| wrong-key/tamper/replay/expired-path checks | PASS | `newmavlink_security_test` |
| nonce reuse وrekey epoch boundary | PASS | `newmavlink_security_test` |
| QoS latest-wins وbounded queue | PASS | `newmavlink_qos_test` |
| subscriptions وcache وdelta base mismatch | PASS | `newmavlink_proxy_test` و`newmavlink_messaging_advanced_test` |
| typed ResyncRequest عند gap وunsubscribe/reorder | PASS | `newmavlink_proxy_test` و`newmavlink_messaging_advanced_test` |
| native Vehicle→Proxy→GCS process path | PASS | `newmavlink_process_e2e` |
| proxy session termination وإعادة التشفير | PASS | process E2E log: `NEWMAVLINK_PROXY_REENCRYPT=PASS` |
| MAVLink2 command allowlist boundary | PASS | `newmavlink_adapter_test` |
| MAVLink2 Position/Attitude typed decoders | PASS | `newmavlink_adapter_test` |
| deterministic bounded parser fuzz | PASS | 100000 iterations في `newmavlink_fuzz_test` |
| messaging advanced: snapshot/delta/reorder/resync/unsubscribe | PASS | `newmavlink_messaging_advanced_test` |
| publisher on-change وhelpful full snapshot كل 10000ms | PASS | `newmavlink_publisher_test` و`newmavlink_publisher_demo`؛ snapshots عند 10000 و20000ms |
| native stream Vehicle→Proxy→GCS لمدة 20.5 ثانية | PASS | 10 رسائل على السلك: 4 on-change و6 helpful snapshots؛ Proxy أعاد تشفير 10، وGCS استقبل 10 typed states، وكل process status=0 |
| Debug CTest | PASS | 11/11 |
| Release CTest | PASS | 11/11 |
| ASan/UBSan CTest | PASS | 11/11، مع `detect_leaks=1` و`halt_on_error=1` |
| PX4 SITL regression من هذه الشجرة | PASS | probe من `build-final-debug2` أعاد `NEWMAVLINK_PX4_ADAPTER_ACK=PASS command=400 status=0` مقابل PX4 SITL فعلي |
| QGroundControl native new_mavlink UI | NOT RUN | لا يوجد QGroundControl executable/AppImage في البيئة الحالية؛ كما أن adapter boundary لا يجعله native client |

## ملاحظات تدقيق

نجاح native E2E لا يثبت PX4 الحقيقي وحده. في هذه الجولة تم تشغيل `newmavlink_px4_probe` مقابل PX4 SITL الموجود فعليًا، وظهر `NEWMAVLINK_PX4_ADAPTER_ACK=PASS command=400 status=0`. هذا يثبت مسار command/ACK للـadapter، ولا يثبت أن QGC أصبح native new_mavlink client.

المفاتيح داخل التطبيقات simulation-only. لم يُدّعَ هنا production provisioning أو PKI أو forward secrecy أو flight readiness.
