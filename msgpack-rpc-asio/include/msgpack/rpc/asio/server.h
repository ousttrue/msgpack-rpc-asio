#pragma once

namespace msgpack {
namespace rpc {
namespace asio {


class server
{
    boost::asio::io_service &m_io_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::list<std::weak_ptr<session>> m_sessions;
    std::shared_ptr<received_msg_queue> m_request_queue;
public:
    server(boost::asio::io_service &io_service)
        : m_io_service(io_service), m_acceptor(io_service), m_request_queue(new received_msg_queue)
    {
    }

    ~server()
    {
        for(auto it=m_sessions.begin(); it!=m_sessions.end(); ++it){
            auto session=it->lock();
            if(session){
                session->socket().close();
            }
        }
    }

    std::shared_ptr<received_msg_queue> get_request_queue(){ return m_request_queue; }

    void listen(boost::asio::ip::tcp::endpoint endpoint)
    {
        m_acceptor.open(endpoint.protocol());
        m_acceptor.bind(endpoint);
        m_acceptor.listen();
        start_accept();
    }

    void stop()
    {
        m_acceptor.close();
    }

private:
    void start_accept()
    {
        auto self=this;
        auto queue=m_request_queue;
        auto new_connection = session::create(m_io_service, [queue](const object &msg, std::shared_ptr<session> session){
					queue->enqueue(std::make_shared<msg_item>(msg, session));
                });

        m_sessions.push_back(new_connection);
        m_acceptor.async_accept(new_connection->socket(),
                [self, new_connection](const boost::system::error_code& error){
                if (error){
                    throw rpc_error("fail to accept ?");
                }
                else{
                    new_connection->start_read();
                    // next
                    self->start_accept();
                }
                });
    }
};

}}}