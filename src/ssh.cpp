#include "ssh.h"

std::ostream &operator<<(std::ostream &out, SshConnData const &s) {
    return out << s.ip.length() << ";" << s.ip << ";" 
                << s.user.length() << ";" << s.user << ";"
                << s.pass.length() << ";" << s.pass << ";"
                << s.path.length() << ";" << s.path << ";";
}

std::istream &operator>>(std::istream &in, SshConnData &s) {
    char separator;
    std::size_t length;
    bool ok = true;
    std::vector<std::string *> v { &s.ip, &s.user, &s.pass, &s.path };
    for (std::size_t i = 0; i < v.size(); i++) {
        ok &= in >> length && in >> separator && separator == ';';
        if (ok) {
            v[i]->resize(length);
            in.read(v[i]->data(), length);
            v[i]->resize(in.gcount());
            ok &= in >> separator && separator == ';' && (v[i]->length() == length);
        }
        if (!ok) {
            break;
        }
    }
    if (!ok) in.setstate(std::ios::failbit);
    return in;
}