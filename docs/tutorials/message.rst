消息
====

前一个教程中提到了 mirai-api-http 包含很多种事件，但是写群聊 bot 的时候我们用得最多的就应该是几种消息事件了。除了处理消息，发送各种各样的消息一般也是 QQ bot 不可缺少的要素。这个教程中我就来讲一讲消息的构建和处理方式。

QQ 中的一条消息可以包含很多种元素，包括文字、图片、语音、at 消息、表情包等等。所有这些种类的元素都对应 Mirai++ 中的一个类型，比如前面的这些就对应着 ``mpp::Plain``, ``mpp::Image``, ``mpp::Voice``, ``mpp::At``, ``mpp::Face`` 这些类型。同时，Mirai++ 中还有一个统一的消息段类型 ``mpp::Segment`` 用来存放任意的消息元素。因为一条消息就是多个消息段的连接，Mirai++ 中的消息类型 ``mpp::Message`` 就是在 ``std::vector<mpp::Segment>`` 之上封装而成的。

.. warning::
    建设中……

1. 消息段
---------



2. 消息构建
-----------

前面提到了 ``send_message`` 函数，这个函数就接受 ``mpp::Message`` 类型的参数作为要发送的消息内容。在上一个教程中我已经展示了这个函数的基本使用方法，这里再来回顾一下直接发送纯文字消息的方法：

.. code-block:: cpp

    // MessageId Bot::send_message(UserId, const Message&);
    bot.send_message(qq, "Hello world!");

可以看到这里函数接受的是 ``mpp::Message``，但是我们传递普通的字符串作为参数也是可行的。``mpp::Message`` 对象可以由任意多个可转换成 ``mpp::Segment`` 的参数来构建，也就是说所有的消息段类型加上各种字符串相关类型以及 ``mpp::Segment`` 本身都可以用来构造 ``mpp::Message``，可以说是基本上写成什么样都能用了。下面就给出几个例子：

.. code-block:: cpp

    mpp::Message plain = "Hello world!";                // 纯文本消息，由字符串构建
    mpp::Message plain2 = mpp::Plain("Hello world!");   // 构建 Plain 再转换 Message
    mpp::Message at_me = mpp::At{ .target = bot.id() }; // at 消息也可以
    mpp::Message multi
    {
        mpp::Image{ .url = "https://github.com/mamoe/mirai/blob/dev/docs/mirai.png" },
        "你们要的美少女图.png"
    };                                                  // 图文并茂
