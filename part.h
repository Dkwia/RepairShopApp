#ifndef PART_H
#define PART_H

#include <QString>

class Part {
public:
    Part(const QString& article, const QString& name, int quantity = 0, double price = 0.0);

    QString article() const;
    QString name() const;
    int quantity() const;
    double price() const;          
    void setQuantity(int q);
    void setPrice(double p);       
    void use(int amount);

private:
    QString m_article;
    QString m_name;
    int m_quantity;
    double m_price = 0.0; 
};

#endif 
