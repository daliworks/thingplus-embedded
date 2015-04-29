## id should be url unreserved characters

RFC 3986 section 2.3 Unreserved Characters (January 2005)
http://tools.ietf.org/html/rfc3986#section-2.3
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
a b c d e f g h i j k l m n o p q r s t u v w x y z
0 1 2 3 4 5 6 7 8 9 - _ . ~

## sensor id template format
```
"idTemplate": "{gatewayId}-{deviceAddress}-{seqence}"

gatewayId: uniq gateway id(currently mac address)
deviceAddress: device address (e.g. bluetooth address(globally uniq), i2c address(locally uniq))
seqence: uniq sequence number in the same device(integar staring from 0)
```


## device id template format
```
"idTemplate": "{gatewayId}-{deviceAddress}"

gatewayId: uniq gateway id(currently mac address)
deviceAddress: device address (e.g. bluetooth address(globally uniq), i2c address(locally uniq))
```
