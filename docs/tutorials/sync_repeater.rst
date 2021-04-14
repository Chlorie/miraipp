人类的本质
==========

1. 会话创建与鉴权
-----------------

我们从一个最简单的例子出发，开始探索 Mirai++ 提供的接口。

众所周知，人类的本质是复读机（确信）。在这个教程里面我们就来实现一个简单的人类本质 bot。

.. code-block:: cpp

    #include <mirai/mirai.h>

    int main()
    {

        return 0;
    }

Mirai++ 的全部 API 都可以通过包含 ``mirai/mirai.h`` 来使用。库中还提供了分组件的头文件 ``mirai/core.h``, ``mirai/event.h``, ``mirai/message.h``。如果觉得包含的东西太多拖慢编译时间的话可以自行选择单个的头文件，不过个人测试的时候即使包含全部的 ``mirai/mirai.h`` 也并没有对编译时长的太大影响。

.. attention::
    如果采用静态链接库编译 Mirai++ 的话在某些编译选项下库文件可能会非常大（~90MB），这时候链接 ``miraipp.lib`` 会耗费很长的时间（可能会需要 20s 左右）。如果对这点不爽的话可以选择把 Mirai++ 编译成动态链接库。

用户的 QQ 号与群号虽然本质上都只是普通的 64 位整数，但是在使用的时候如果类型不加分辨就很容易将错误的 ID 传给函数。为了避免这个问题，Mirai++ 采用了强类型来区分这两种 ID：QQ 号的类型为 ``mpp::UserId``，而群号的类型是 ``mpp::GroupId``。这两种类型都可以显式地由 ``int64_t`` 构造。

.. code-block:: cpp

    mpp::UserId null_qq;            // 默认构造的 QQ 号和群号包含 0 值，标识无效的 ID
    mpp::UserId qq{123456789};      // QQ 号 123456789
    // mpp::UserId qq = 123456789;  // 这样不可，必须显式构造
    mpp::GroupId group{123456789};  // 群号 123456789
    // qq = group;                  // 强类型无法直接赋值
    int64_t id = qq.id;             // 可以通过 .id 访问整数值 

也可以用用户定义字面量（UDL）来构造 ID。要使用 UDL 需要先使用对应的命名空间。

.. code-block:: cpp

    using namespace mpp::literals;
    mpp::UserId qq = 123456789_uid;     // _uid 构造 QQ 号
    mpp::GroupId group = 123456789_gid; // _gid 构造群号

mirai-api-http 中几乎所有的操作都需要通过一个有效的会话（session）来发起，在 Mirai++ 中 ``mpp::Bot`` 类就是对此的抽象。首先你需要给出 mirai-api-http 的服务端点来创建一个会话：

.. code-block:: cpp

    mpp::Bot bot("127.0.0.1", "8080");  // 给出主机和端口号

    // mirai-api-http 的默认设置就是采用端点 127.0.0.1:8080
    // 在默认情况可以直接使用 mpp::Bot 构造函数的默认参数
    mpp::Bot bot;                       // 与上面效果相同

连接到端点以后就需要登录 QQ 号，向 mirai-api-http 进行鉴权，这需要使用你配置插件时设置的 auth key 以及 bot 的 QQ 号。注意，要先在 Mirai Console 中登录 bot 的 QQ，Mirai++ 不提供（也无法提供）登录的接口。

.. code-block:: cpp

    bot.authorize("authkey", bot_uid);

鉴权以后就可以让 bot 账号进行消息和事件的处理了。比如说，你可以让 bot 给自己发一条消息试试效果：

.. code-block:: cpp

    bot.send_message(your_uid, "I'm alive!");

2. 使用 WebSocket 接收事件
--------------------------

群消息、好友消息、群内开启匿名、有新成员加群等等这些在 mirai-api-http 中都归为事件（Event）。这里我们要写复读机，就要接收群消息并且对消息内容做出反应。mirai-api-http 提供了两种接收事件的方法：一是通过反复的 http 请求询问是否有新事件（拉取式），二是采用 WebSocket 让服务端在接到新事件以后提醒客户端（推送式）。推送式的代码比较好写且易懂，但 mirai-api-http 会话默认是不开启 WebSocket 的。若要开启 WebSocket，先要进行当前会话的设置。

会话有两个选项可以设置：一是服务端需要缓存多少条消息（cache_size）用于消息的引用回复等功能，二是服务端是否需要开启 WebSocket（enable_websocket）。使用 ``Bot::config`` 函数来设置当前的会话：

.. code-block:: cpp

    mpp::SessionConfig conf1(4096, true);       // 设置缓存 4096 条消息，开启 WebSocket
    bot.config(conf1);                          // 进行设置

    bot.config({ 4096, true });                 // 让编译器进行类型推导，同样效果

    mpp::SessionConfig conf2({}, true);         // 参数留空表示不更改对应的设置项
    bot.config(conf2);

    // C++20 的指派初始化，可读性 up
    mpp::SessionConfig conf3{ .cache_size = 2048, .enable_websocket = true };
    bot.config(conf3);

    bot.config({ .enable_websocket = true });   // 甚至可以这样达到类似具名可选参数的效果，非常 cool

别管是用哪种写法，开启 WebSocket 以后就可以进行消息的监听了。使用 ``Bot::monitor_events`` 就可以监听所有种类的事件，当收到事件时回调函数就会被调用。

.. code-block:: cpp

    // 回调函数需要是 bool callback(const mpp::Event&) 这样的原型
    // 这里我们使用一个 lambda 表达式作为回调函数
    bot.monitor_events([](const mpp::Event& ev)
    {
        // 处理事件 ev
    });

我们这里只想对群消息事件进行处理，而这里 ``monitor_events`` 不论接收何种事件都会调用这个回调函数。我们需要判断 ``ev`` 的实际类型，之后再处理。

.. code-block:: cpp

    // 可以先比较事件类型，之后再用 get 函数获取实际类型的事件
    if (ev.type() == mpp::EventType::group_message)
    {
        const auto& gm = ev.get<mpp::GroupMessageEvent>();
        // 处理群消息事件...
    }

    // 也可以使用 get_if 函数获取指针，若事件类型不匹配就会返回空指针
    if (const auto* ptr = ev.get_if<mpp::GroupMessageEvent>())
    {
        // 处理群消息事件...
    }

每个事件对象都存储了对接收事件的 Bot 的引用，并且提供了一些直接调用 Bot 功能的便捷函数，比如所有群事件都包含 ``send_message`` 成员函数用于直接在对应群里发送消息等等。具体事件类型包含什么成员函数可以参照 API 列表。这里我们直接在对应的群里面发送相同内容的消息。

.. code-block:: cpp

    // content 成员函数用于返回消息内容
    // 如何对消息内容进行操作后面会讲述
    ptr->send_message(ptr->content());

最后，``monitor_events`` 的回调函数要求返回一个 ``bool``，若返回值为 ``false`` 则中止事件监听。这里我们在一般情况返回 ``true`` 继续监听，而当有人发送内容为 stop 的群消息的时候停止监听结束程序。

.. code-block:: cpp

    if (const auto* ptr = ev.get_if<mpp::GroupMessageEvent>())
    {
        // 消息内容可以直接与字符串比较
        if (ptr->content() == "stop") return false;
        ptr->send_message(ptr->content());
    }
    return true;

到此，一个人类本质 bot 就写完了。下面给出完整代码：

.. code-block:: cpp

    #include <mirai/mirai.h>

    int main()
    {
        mpp::Bot bot;
        bot.authorize("your auth key", your_bot_uid);
        bot.config({ .enable_websocket = true });
        bot.monitor_events([](const mpp::Event& ev)
        {
            if (const auto* ptr = ev.get_if<mpp::GroupMessageEvent>())
            {
                if (ptr->content() == "stop") return false;
                ptr->send_message(ptr->content());
            }
            return true;
        });
    }

.. rst-class:: strike

我也不知道一共就不到 20 行代码我怎么扯了这么多
