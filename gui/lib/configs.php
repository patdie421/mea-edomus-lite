<?php
ini_set('error_reporting', E_ERROR);
ini_set('log_errors', 'On');
ini_set('display_errors', 'Off');
ini_set('error_log', "/data/prod/mea-edomus/var/log/php.log");
ini_set('session.save_path', "/data/prod/mea-edomus/var/sessions");
$TITRE_APPLICATION='Mea eDomus Admin';
$BASEPATH='mea-edomus';
$PARAMS_DB_PATH='sqlite:/data/prod/mea-edomus/var/db/params.db';
$QUERYDB_SQL='sql/querydb.sql';
$LANG='fr';
