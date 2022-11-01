# Adding Compression to Db2 Clients

The Db2 client data stream ( DRDA protocol ) is not compressed. If the data is travelling across a WAN or slow network then data volume can introduce significant latency. This is particularly true of activities such as cursor load. ctunnel is a userspace application that will proxy network connections. Compression is supplied by the zlib library.

For Db2 you will want to use a heavily modified version of ctunnel available here:

https://github.com/thaughbaer/ctunnel

For better compression ratios you should compress before you encrypt. ctunnel can encrypt between the client and the server using symmetric encrytion. This uses a shared key on the client and the server. If possible you should colocate ctunnel with the Db2 server and client because traffic between ctunnel and Db2 is not encrypted.

In this example there is a Db2 server listening on host 10.51.9.53 and port 50000.

On the Db2 client start ctunnel in client mode:

```
ctunnel -n -c -b 65536 -S -z 5 -l 0.0.0.0:51234 -f 10.51.9.53:51234 -C proxy
```

On the Db2 server start ctunnel in server mode:
```
ctunnel -n -s -b 65536 -S -z 5 -l 0.0.0.0:51234 -f 127.0.0.1:50000 -C proxy
```

In this case on the Db2 client you would catalog a TCPIP node with host 127.0.0.1 and port 51234:

```
db2 "catalog tcpip node BLUDR remote 127.0.0.1 server 51234"
db2 "catalog database BLUDB as BLUDR at node BLUDR"
```

For applications using the CLI driver ( eg. db2lueff ) you can improve performance by tuning the block fetch size. In db2cli.ini:
```
[common]
;FET_BUF_SIZE=256K
BlockForNRows=40000
BlockLobs=1
```

## Integration With AKS

There is a prebuilt container image which if desired you can push to a private registry here:

https://quay.io/repository/thaughbaer/ctunnel

In this example there is a database BLUDR on host 10.51.9.53 and port 50000.

Update the config map YAML with:
- The metadata name

Update the deployment YAML with:
- The metadata name
- The selector inst
- If using a private registry the image name

Update the service YAML with:
- The metadata name
- The selector inst

Create a namespace:
```
kubectl create namespace ctunnel
```

Update the config map values as required: 

```
bholam:ctunnel$ cat ctunnel-bludr.ConfigMap.yml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ctunnel-bludr
  namespace: ctunnel
data:
  MODE: CLIENT
  BUFFER_SIZE: "65536"
  FORWARD_HOST: 10.51.9.53
  FORWARD_PORT: "51234"
```

Create the configmap:
```
kubectl create -f ctunnel-bludr.ConfigMap.yml
```

Create the deployment:
```
kubectl create -f ctunnel-bludr.Deployment.yml
```

Create a service:
```
kubectl create -f ctunnel-bludr.Deployment.yml
