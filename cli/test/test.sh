echo "--- USER ---"
echo ">>> GET"
python2.7 mea-edomus-cli.py user get

echo ">>> ADD <<<"
python2.7 mea-edomus-cli.py user add toto password:toto profile:1 fullname:"tata tat"
python2.7 mea-edomus-cli.py user add tata password:toto profile:0
echo "=== result ==="
python2.7 mea-edomus-cli.py user get

echo ">>> UPDATE <<<"
python2.7 mea-edomus-cli.py user update toto fullname:"tata de toto"
echo "=== result ==="
python2.7 mea-edomus-cli.py user get

echo ">>> DELETE <<<"
python2.7 mea-edomus-cli.py user delete toto
echo "=== result ==="
python2.7 mea-edomus-cli.py user get

echo ">>> ROLLBACK <<<"
python2.7 mea-edomus-cli.py user rollback
echo "=== result ==="
python2.7 mea-edomus-cli.py user get

echo
echo
echo "--- SERVICE ---"
echo ">>> GET"
python2.7 mea-edomus-cli.py service get

echo ">>> STOP <<<"
python2.7 mea-edomus-cli.py service stop 3
echo "=== result ==="
python2.7 mea-edomus-cli.py service get


echo ">>> START <<<"
python2.7 mea-edomus-cli.py service start 3
echo "=== result ==="
python2.7 mea-edomus-cli.py service get

echo
echo
echo "--- INTERFACE ---"
echo ">>> GET"
python2.7 mea-edomus-cli.py interface get

echo "--- ADD"
python mea-edomus-cli.py interface add IXXX id_type:TYP010 state:disabled dev:"I010://00-00-00-00"
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX

echo "--- UPDATE"
python mea-edomus-cli.py interface update IXXX description:"un test"
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX

echo "--- ADD DEVICE"
python mea-edomus-cli.py interface add IXXX device IXXX-001 id_type:OUTPUT state:disabled 
python mea-edomus-cli.py interface add IXXX device IXXX-002 id_type:INPUT state:disabled 
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX

echo "--- DELETE DEVICE"
python mea-edomus-cli.py interface delete IXXX device IXXX-001
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX

echo "--- UPDATE DEVICE"
python mea-edomus-cli.py interface update IXXX device IXXX-002 description:"device keeped"
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX

echo "--- DELETE INTERFACE"
python mea-edomus-cli.py interface delete IXXX
echo "=== result ==="
python2.7 mea-edomus-cli.py interface get IXXX
