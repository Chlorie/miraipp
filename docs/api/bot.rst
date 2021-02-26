Bot
===

.. doxygenclass:: mpp::Bot

公共成员函数
------------

特殊成员函数
............
.. doxygenfunction:: mpp::Bot::Bot(const std::string_view, const std::string_view)
.. doxygenfunction:: mpp::Bot::~Bot

mirai-api-http 会话管理
...........................
.. doxygengroup:: bot_get_version
   :content-only:
.. doxygengroup:: bot_authorize
   :content-only:
.. doxygenfunction:: mpp::Bot::release
.. doxygenfunction:: mpp::Bot::async_release

消息发送
........
.. doxygengroup:: bot_send_message
   :content-only:
