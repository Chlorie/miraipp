Message
=======

.. doxygenclass:: mpp::Message

公共成员函数
------------

消息段访问
..........
.. doxygengroup:: MsgIdx
   :content-only:

连接消息
........
.. note::
   避免在同一个消息对象上重复调用 ``+`` 运算符。若要构建多段的消息请直接使用构造函数。

.. function:: template <typename T> requires std::convertible_to<T&&, Segment>\
              Message& operator+=(T&& segment)

   在本消息末尾添加一个消息段

相等比较
........
.. doxygengroup:: MsgCmp
   :content-only:
