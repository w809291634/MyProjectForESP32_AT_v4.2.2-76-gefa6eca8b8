.. _MQTT-AT:

MQTT AT Commands
================

:link_to_translation:`en:[English]`

-  :ref:`AT+MQTTUSERCFG <cmd-MQTTUSERCFG>`：设置 MQTT 用户属性
-  :ref:`AT+MQTTCLIENTID <cmd-MQTTCLIENTID>`：设置 MQTT 客户端 ID
-  :ref:`AT+MQTTUSERNAME <cmd-MQTTUSERNAME>`：设置 MQTT 登陆用户名
-  :ref:`AT+MQTTPASSWORD <cmd-MQTTPASSWORD>`：设置 MQTT 登陆密码
-  :ref:`AT+MQTTCONNCFG <cmd-MQTTCONNCFG>`：设置 MQTT 连接属性
-  :ref:`AT+MQTTCONN <cmd-MQTTCONN>`：连接 MQTT Broker
-  :ref:`AT+MQTTPUB <cmd-MQTTPUB>`：发布 MQTT 消息（字符串）
-  :ref:`AT+MQTTPUBRAW <cmd-MQTTPUBRAW>`：发布 MQTT 消息（二进制）
-  :ref:`AT+MQTTSUB <cmd-MQTTSUB>`：订阅 MQTT Topic
-  :ref:`AT+MQTTUNSUB <cmd-MQTTUNSUB>`：取消订阅 MQTT Topic
-  :ref:`AT+MQTTCLEAN <cmd-MQTTCLEAN>`：断开 MQTT 连接
-  :ref:`MQTT AT 错误码 <MQTTErrCod>`
-  :ref:`MQTT AT 说明 <MQTTNote>`

.. _cmd-MQTTUSERCFG:

:ref:`AT+MQTTUSERCFG <MQTT-AT>`：设置 MQTT 用户属性
-------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

配置 MQTT 用户属性

**命令：**

::

    AT+MQTTUSERCFG=<LinkID>,<scheme>,<"client_id">,<"username">,<"password">,<cert_key_ID>,<CA_ID>,<"path">

**响应：**

::

   OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<scheme>**：

   -  1: MQTT over TCP；
   -  2: MQTT over TLS（不校验证书）；
   -  3: MQTT over TLS（校验 server 证书）；
   -  4: MQTT over TLS（提供 client 证书）；
   -  5: MQTT over TLS（校验 server 证书并且提供 client 证书）；
   -  6: MQTT over WebSocket（基于 TCP）；
   -  7: MQTT over WebSocket Secure（基于 TLS，不校验证书）；
   -  8: MQTT over WebSocket Secure（基于 TLS，校验 server 证书）；
   -  9: MQTT over WebSocket Secure（基于 TLS，提供 client 证书）；
   -  10: MQTT over WebSocket Secure（基于 TLS，校验 server 证书并且提供 client 证书）。

-  **<client_id>**：MQTT 客户端 ID，最大长度：256 字节。
-  **<username>**：用户名，用于登陆 MQTT broker，最大长度：64 字节。
-  **<password>**：密码，用于登陆 MQTT broker，最大长度：64 字节。
-  **<cert_key_ID>**：证书 ID，目前 ESP-AT 仅支持一套 cert 证书，参数为 0。
-  **<CA_ID>**：CA ID，目前 ESP-AT 仅支持一套 CA 证书，参数为 0。
-  **<path>**：资源路径，最大长度：32 字节。

说明
^^^^

-  每条 AT 命令的总长度不能超过 256 字节。

.. _cmd-MQTTCLIENTID:

:ref:`AT+MQTTCLIENTID <MQTT-AT>`：设置 MQTT 客户端 ID
--------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

设置 MQTT 客户端 ID

**命令：**

::

    AT+MQTTCLIENTID=<LinkID>,<"client_id">

**响应：**

::

   OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<client_id>**：MQTT 客户端 ID。

说明
^^^^

-  每条 AT 命令的总长度不能超过 256 字节。
-  :ref:`AT+MQTTUSERCFG <cmd-MQTTUSERCFG>` 命令也可以设置 MQTT 客户端 ID，二者之间的差别包括：

   - ``AT+MQTTCLIENTID`` 命令可以用来设置相对较长的客户端 ID，因为 ``AT+MQTTUSERCFG`` 命令的长度受限；
   - 应在设置 ``AT+MQTTUSERCFG`` 后再使用 ``AT+MQTTCLIENTID``。

.. _cmd-MQTTUSERNAME:

:ref:`AT+MQTTUSERNAME <MQTT-AT>`：设置 MQTT 登陆用户名
----------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

设置 MQTT 用户名

**命令：**

::

    AT+MQTTUSERNAME=<LinkID>,<"username">

**响应：**

::

   OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<username>**：用于登陆 MQTT broker 的用户名。

说明
^^^^

-  每条 AT 命令的总长度不能超过 256 字节。
-  :ref:`AT+MQTTUSERCFG <cmd-MQTTUSERCFG>` 命令也可以设置 MQTT 用户名，二者之间的差别包括：

   - ``AT+MQTTUSERNAME`` 命令可以用来设置相对较长的用户名，因为 ``AT+MQTTUSERCFG`` 命令的长度受限。
   - 应在设置 ``AT+MQTTUSERCFG`` 后再使用 ``AT+MQTTUSERNAME``。

.. _cmd-MQTTPASSWORD:

:ref:`AT+MQTTPASSWORD <MQTT-AT>`：设置 MQTT 登陆密码
-------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

设置 MQTT 密码

**命令：**

::

    AT+MQTTPASSWORD=<LinkID>,<"password">

**响应：**

::

   OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<password>**：用于登陆 MQTT broker 的密码。

说明
^^^^

-  每条 AT 命令的总长度不能超过 256 字节。
-  :ref:`AT+MQTTUSERCFG <cmd-MQTTUSERCFG>` 命令也可以设置 MQTT 密码，二者之间的差别包括：

   - ``AT+MQTTPASSWORD`` 可以用来设置相对较长的密码，因为 ``AT+MQTTUSERCFG`` 命令的长度受限；
   - 应在设置 ``AT+MQTTUSERCFG`` 后再使用 ``AT+MQTTPASSWORD``。

.. _cmd-MQTTCONNCFG:

:ref:`AT+MQTTCONNCFG <MQTT-AT>`：设置 MQTT 连接属性
-------------------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

设置 MQTT 连接属性

**命令：**

::

    AT+MQTTCONNCFG=<LinkID>,<keepalive>,<disable_clean_session>,<"lwt_topic">,<"lwt_msg">,<lwt_qos>,<lwt_retain>

**响应：**

::

   OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<keepalive>**：MQTT ping 超时时间，单位：秒。范围：[0,7200]。默认值：0，会被强制改为 120 秒。
-  **<disable_clean_session>**：设置 MQTT 清理会话标志，有关该参数的更多信息请参考 MQTT 3.1.1 协议中的 `Clean Session <http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf>`_ 章节。

   -  0: 使能清理会话
   -  1: 禁用清理会话

-  **<lwt_topic>**：遗嘱 topic，最大长度：128 字节。
-  **<lwt_msg>**：遗嘱 message，最大长度：64 字节。
-  **<lwt_qos>**：遗嘱 QoS，参数可选 0、1、2，默认值：0。
-  **<lwt_retain>**：遗嘱 retain，参数可选 0 或 1，默认值：0。

.. _cmd-MQTTCONN:

:ref:`AT+MQTTCONN <MQTT-AT>`：连接 MQTT Broker
--------------------------------------------------------

查询命令
^^^^^^^^

**功能：**

查询 ESP 设备已连接的 MQTT broker

**命令：**

::

    AT+MQTTCONN?

**响应：**

::

    +MQTTCONN:<LinkID>,<state>,<scheme><"host">,<port>,<"path">,<reconnect>
    OK

设置命令
^^^^^^^^

**功能：**

连接 MQTT Broker 

**命令：**

::

    AT+MQTTCONN=<LinkID>,<"host">,<port>,<reconnect>

**响应：**

::

    OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<host>**：MQTT broker 域名，最大长度：128 字节。
-  **<port>**：MQTT broker 端口，最大端口：65535。
-  **<path>**：资源路径，最大长度：32 字节。
-  **<reconnect>**：

   -  0: MQTT 不自动重连；
   -  1: MQTT 自动重连，会消耗较多的内存资源。

-  **<state>**：MQTT 状态：

   -  0: MQTT 未初始化；
   -  1: 已设置 ``AT+MQTTUSERCFG``；
   -  2: 已设置 ``AT+MQTTCONNCFG``；
   -  3: 连接已断开；
   -  4: 已建立连接；
   -  5: 已连接，但未订阅 topic；
   -  6: 已连接，已订阅过 topic。

-  **<scheme>**：

   -  1: MQTT over TCP；
   -  2: MQTT over TLS（不校验证书）；
   -  3: MQTT over TLS（校验 server 证书）；
   -  4: MQTT over TLS（提供 client 证书）；
   -  5: MQTT over TLS（校验 server 证书并且提供 client 证书）；
   -  6: MQTT over WebSocket（基于 TCP）；
   -  7: MQTT over WebSocket Secure（基于 TLS，不校验证书）；
   -  8: MQTT over WebSocket Secure（基于 TLS，校验 server 证书）；
   -  9: MQTT over WebSocket Secure（基于 TLS，提供 client 证书）；
   -  10: MQTT over WebSocket Secure（基于 TLS，校验 server 证书并且提供 client 证书）。

.. _cmd-MQTTPUB:

:ref:`AT+MQTTPUB <MQTT-AT>`：发布 MQTT 消息（字符串）
---------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

通过 topic 发布 MQTT **字符串** 消息，若要发布 **二进制** 消息，请使用 :ref:`AT+MQTTPUBRAW <cmd-MQTTPUBRAW>` 命令。

**命令：**

::

    AT+MQTTPUB=<LinkID>,<"topic">,<"data">,<qos>,<retain>

**响应：**

::

    OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<topic>**：MQTT topic，最大长度：128 字节。
-  **<data>**：MQTT 字符串消息。
-  **<qos>**：发布消息的 QoS，参数可选 0、1、或 2，默认值：0。
-  **<retain>**：发布 retain。

说明
^^^^

-  每条 AT 命令的总长度不能超过 256 字节。
-  本命令不能发送数据 ``\0``，若需要发送该数据，请使用 :ref:`AT+MQTTPUBRAW <cmd-MQTTPUBRAW>` 命令。

.. _cmd-MQTTPUBRAW:

:ref:`AT+MQTTPUBRAW <MQTT-AT>`：发布 MQTT 消息（二进制）
------------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

通过 topic 发布 MQTT **二进制** 消息

**命令：**

::

    AT+MQTTPUBRAW=<LinkID>,<"topic">,<length>,<qos>,<retain>

**响应：**

::

    OK
    > 

符号 ``>`` 表示 AT 准备好接收串口数据，此时您可以输入数据，当数据长度达到参数 ``<length>`` 的值时，数据传输开始。

若传输成功，则 AT 返回：

::

    +MQTTPUB:OK

若传输失败，则 AT 返回：

::

    +MQTTPUB:FAIL

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<topic>**：MQTT topic，最大长度：128 字节。
-  **<length>**：MQTT 消息长度，不同 ESP 设备的最大长度不同：

   -  对于 ESP32 设备：最大长度受到可利用内存的限制；
   -  对于 ESP8266 设备，最大长度受到可利用内存和 ``MQTT_BUFFER_SIZE_BYTE`` 宏的限制。该宏的默认值为 ``512``，可在 ``build.py menuconfig`` 中设置它的值，以此更改对最大长度的限制。该宏的值 = 消息的最大长度 + MQTT 报头长度（取决于 topic 名称的长度）。

-  **<qos>**：发布消息的 QoS，参数可选 0、1、或 2，默认值：0。
-  **<retain>**：发布 retain。

.. _cmd-MQTTSUB:

:ref:`AT+MQTTSUB <MQTT-AT>`：订阅 MQTT Topic
--------------------------------------------------------

查询命令
^^^^^^^^

**功能：**

查询已订阅的 topic

**命令：**

::

    AT+MQTTSUB?    


**响应：**

::

    +MQTTSUB:<LinkID>,<state>,<"topic1">,<qos>
    +MQTTSUB:<LinkID>,<state>,<"topic2">,<qos>
    +MQTTSUB:<LinkID>,<state>,<"topic3">,<qos>
    ...
    OK

设置命令
^^^^^^^^

**功能：**

订阅指定 MQTT topic 的指定 QoS，支持订阅多个 topic

**命令：**

::

    AT+MQTTSUB=<LinkID>,<"topic">,<qos>


**响应：**

::

    OK

当 AT 接收到已订阅的 topic 的 MQTT 消息时，返回：

::

    +MQTTSUBRECV:<LinkID>,<"topic">,<data_length>,data

若已订阅过该 topic，则返回：

::

   ALREADY SUBSCRIBE

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<state>**：MQTT 状态：

   -  0: MQTT 未初始化；
   -  1: 已设置 ``AT+MQTTUSERCFG``；
   -  2: 已设置 ``AT+MQTTCONNCFG``；
   -  3: 连接已断开；
   -  4: 已建立连接；
   -  5: 已连接，但未订阅 topic；
   -  6: 已连接，已订阅过 MQTT topic。
-  **<topic>**：订阅的 topic。
-  **<qos>**：订阅的 QoS。

.. _cmd-MQTTUNSUB:

:ref:`AT+MQTTUNSUB <MQTT-AT>`：取消订阅 MQTT Topic
--------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

客户端取消订阅指定 topic，可多次调用本命令，以取消订阅不同的 topic。

**命令：**

::

    AT+MQTTUNSUB=<LinkID>,<"topic">


**响应：**

::

    OK

若未订阅过该 topic，则返回：

::

  NO UNSUBSCRIBE 
  
  OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。
-  **<topic>**：MQTT topic，最大长度：128 字节。

.. _cmd-MQTTCLEAN:

:ref:`AT+MQTTCLEAN <MQTT-AT>`：断开 MQTT 连接
------------------------------------------------------------

设置命令
^^^^^^^^

**功能：**

断开 MQTT 连接，释放资源。

**命令：**

::

    AT+MQTTCLEAN=<LinkID>  

**响应：**

::

    OK

参数
^^^^

-  **<LinkID>**：当前仅支持 link ID 0。

.. _MQTTErrCod:

:ref:`MQTT AT 错误码 <MQTT-AT>`
-------------------------------------

MQTT 错误码以 ``ERR CODE:0x<%08x>`` 形式打印。

.. list-table::
   :header-rows: 1
   :widths: 15 5

   * - 错误类型
     - 错误码
   * - AT_MQTT_NO_CONFIGURED
     - 0x6001
   * - AT_MQTT_NOT_IN_CONFIGURED_STATE
     - 0x6002
   * - AT_MQTT_UNINITIATED_OR_ALREADY_CLEAN
     - 0x6003
   * - AT_MQTT_ALREADY_CONNECTED
     - 0x6004
   * - AT_MQTT_MALLOC_FAILED
     - 0x6005
   * - AT_MQTT_NULL_LINK
     - 0x6006
   * - AT_MQTT_NULL_PARAMTER
     - 0x6007
   * - AT_MQTT_PARAMETER_COUNTS_IS_WRONG
     - 0x6008
   * - AT_MQTT_TLS_CONFIG_ERROR
     - 0x6009
   * - AT_MQTT_PARAM_PREPARE_ERROR
     - 0x600A
   * - AT_MQTT_CLIENT_START_FAILED
     - 0x600B
   * - AT_MQTT_CLIENT_PUBLISH_FAILED
     - 0x600C
   * - AT_MQTT_CLIENT_SUBSCRIBE_FAILED
     - 0x600D
   * - AT_MQTT_CLIENT_UNSUBSCRIBE_FAILED
     - 0x600E
   * - AT_MQTT_CLIENT_DISCONNECT_FAILED
     - 0x600F
   * - AT_MQTT_LINK_ID_READ_FAILED
     - 0x6010
   * - AT_MQTT_LINK_ID_VALUE_IS_WRONG
     - 0x6011
   * - AT_MQTT_SCHEME_READ_FAILED
     - 0x6012
   * - AT_MQTT_SCHEME_VALUE_IS_WRONG
     - 0x6013
   * - AT_MQTT_CLIENT_ID_READ_FAILED
     - 0x6014
   * - AT_MQTT_CLIENT_ID_IS_NULL
     - 0x6015
   * - AT_MQTT_CLIENT_ID_IS_OVERLENGTH
     - 0x6016
   * - AT_MQTT_USERNAME_READ_FAILED
     - 0x6017
   * - AT_MQTT_USERNAME_IS_NULL
     - 0x6018
   * - AT_MQTT_USERNAME_IS_OVERLENGTH
     - 0x6019
   * - AT_MQTT_PASSWORD_READ_FAILED
     - 0x601A
   * - AT_MQTT_PASSWORD_IS_NULL
     - 0x601B
   * - AT_MQTT_PASSWORD_IS_OVERLENGTH
     - 0x601C
   * - AT_MQTT_CERT_KEY_ID_READ_FAILED
     - 0x601D
   * - AT_MQTT_CERT_KEY_ID_VALUE_IS_WRONG
     - 0x601E
   * - AT_MQTT_CA_ID_READ_FAILED
     - 0x601F
   * - AT_MQTT_CA_ID_VALUE_IS_WRONG
     - 0x6020
   * - AT_MQTT_CA_LENGTH_ERROR
     - 0x6021
   * - AT_MQTT_CA_READ_FAILED
     - 0x6022
   * - AT_MQTT_CERT_LENGTH_ERROR
     - 0x6023
   * - AT_MQTT_CERT_READ_FAILED
     - 0x6024
   * - AT_MQTT_KEY_LENGTH_ERROR
     - 0x6025
   * - AT_MQTT_KEY_READ_FAILED
     - 0x6026
   * - AT_MQTT_PATH_READ_FAILED
     - 0x6027
   * - AT_MQTT_PATH_IS_NULL
     - 0x6028
   * - AT_MQTT_PATH_IS_OVERLENGTH
     - 0x6029
   * - AT_MQTT_VERSION_READ_FAILED
     - 0x602A
   * - AT_MQTT_KEEPALIVE_READ_FAILED
     - 0x602B
   * - AT_MQTT_KEEPALIVE_IS_NULL
     - 0x602C
   * - AT_MQTT_KEEPALIVE_VALUE_IS_WRONG
     - 0x602D
   * - AT_MQTT_DISABLE_CLEAN_SESSION_READ_FAILED
     - 0x602E
   * - AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG
     - 0x602F
   * - AT_MQTT_LWT_TOPIC_READ_FAILED
     - 0x6030
   * - AT_MQTT_LWT_TOPIC_IS_NULL
     - 0x6031
   * - AT_MQTT_LWT_TOPIC_IS_OVERLENGTH
     - 0x6032
   * - AT_MQTT_LWT_MSG_READ_FAILED
     - 0x6033
   * - AT_MQTT_LWT_MSG_IS_NULL
     - 0x6034
   * - AT_MQTT_LWT_MSG_IS_OVERLENGTH
     - 0x6035
   * - AT_MQTT_LWT_QOS_READ_FAILED
     - 0x6036
   * - AT_MQTT_LWT_QOS_VALUE_IS_WRONG
     - 0x6037
   * - AT_MQTT_LWT_RETAIN_READ_FAILED
     - 0x6038
   * - AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG
     - 0x6039
   * - AT_MQTT_HOST_READ_FAILED
     - 0x603A
   * - AT_MQTT_HOST_IS_NULL
     - 0x603B
   * - AT_MQTT_HOST_IS_OVERLENGTH
     - 0x603C
   * - AT_MQTT_PORT_READ_FAILED
     - 0x603D
   * - AT_MQTT_PORT_VALUE_IS_WRONG
     - 0x603E
   * - AT_MQTT_RECONNECT_READ_FAILED
     - 0x603F
   * - AT_MQTT_RECONNECT_VALUE_IS_WRONG
     - 0x6040
   * - AT_MQTT_TOPIC_READ_FAILED
     - 0x6041
   * - AT_MQTT_TOPIC_IS_NULL
     - 0x6042
   * - AT_MQTT_TOPIC_IS_OVERLENGTH
     - 0x6043
   * - AT_MQTT_DATA_READ_FAILED
     - 0x6044
   * - AT_MQTT_DATA_IS_NULL
     - 0x6045
   * - AT_MQTT_DATA_IS_OVERLENGTH
     - 0x6046
   * - AT_MQTT_QOS_READ_FAILED
     - 0x6047
   * - AT_MQTT_QOS_VALUE_IS_WRONG
     - 0x6048
   * - AT_MQTT_RETAIN_READ_FAILED
     - 0x6049
   * - AT_MQTT_RETAIN_VALUE_IS_WRONG
     - 0x604A
   * - AT_MQTT_PUBLISH_LENGTH_READ_FAILED
     - 0x604B
   * - AT_MQTT_PUBLISH_LENGTH_VALUE_IS_WRONG
     - 0x604C
   * - AT_MQTT_RECV_LENGTH_IS_WRONG
     - 0x604D
   * - AT_MQTT_CREATE_SEMA_FAILED
     - 0x604E
   * - AT_MQTT_CREATE_EVENT_GROUP_FAILED
     - 0x604F
   * - AT_MQTT_URI_PARSE_FAILED
     - 0x6050
   * - AT_MQTT_IN_DISCONNECTED_STATE
     - 0x6051
   * - AT_MQTT_HOSTNAME_VERIFY_FAILED
     - 0x6052

.. _MQTTNote:

:ref:`MQTT AT 说明 <MQTT-AT>`
-------------------------------

-  一般来说，AT MQTT 命令都会在 10 秒内响应，但 ``AT+MQTTCONN`` 命令除外。例如，如果路由器不能上网，命令 ``AT+MQTTPUB`` 会在 10 秒内响应，但 ``AT+MQTTCONN`` 命令在网络环境不好的情况下，可能需要更多的时间用来重传数据包。
-  如果 ``AT+MQTTCONN`` 是基于 TLS 连接，每个数据包的超时时间为 10 秒，则总超时时间会根据握手数据包的数量而变得更长。
-  当 MQTT 连接断开时，会提示 ``+MQTTDISCONNECTED:<LinkID>`` 消息。
-  当 MQTT 连接建立时，会提示 ``+MQTTCONNECTED:<LinkID>,<scheme>,<"host">,port,<"path">,<reconnect>`` 消息。
