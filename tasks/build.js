/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 */
/*jslint vars:true, regexp:true*/
/*global module, require, process*/

module.exports = function (grunt) {
    "use strict";
    
    var common      = require("./common")(grunt),
        q           = require("q"),
        exec        = common.exec,
        pipe        = common.pipe,
        spawn       = common.spawn,
        resolve     = common.resolve,
        platform    = common.platform;
    
    // task: full-build
    grunt.registerTask(["update-repo", "build", "installer"]);
    
    // task: build
    grunt.registerTask("build", "Build shell executable. Run 'grunt full-build' to update repositories, build the shell, and build an installer.", function (wwwBranch, shellBranch) {
        grunt.task.run("build-" + platform());
    });
    
    // task: build-mac
    grunt.registerTask("build-mac", "Build mac shell", function () {
        var done = this.async();
        
        spawn([
            "xcodebuild -project appshell.xcodeproj -config Release clean",
            "xcodebuild -project appshell.xcodeproj -config Release build"
        ]).then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: build-win
    grunt.registerTask("build-win", "Build windows shell", function () {
        var done = this.async();
        
        spawn("cmd.exe /c scripts\\build_projects.bat").then(function () {
            done();
        }, function (err) {
            grunt.log.error(err);
            done(false);
        });
    });
    
    // task: update-repo
    grunt.registerMultiTask("update-repo", "Pull specified www-repo branch from origin", function () {
        var repo = this.data.repo;
        
        if (!repo) {
            grunt.fail.fatal("Missing repo config");
        }
        
        repo = resolve(repo);
        
        if (this.data.branch) {
            grunt.verbose.writeln("Updating repo " + this.target + " at " + repo);
            
            var done = this.async(),
                promise =
                    spawn([
                        "git checkout " + this.data.branch,
                        "git pull origin " + this.data.branch,
                        "git submodule update --init --recursive"
                    ], { cwd: repo });
        
            promise.then(function () {
                done();
            }, function (err) {
                grunt.log.writeln(err);
                done(false);
            });
        } else {
            grunt.log.writeln("Skipping fetch for " + this.target + " repo");
        }
    });
    
    grunt.registerTask("build-num", "Compute www-repo build number and set config property build.build-number", function () {
        var done = this.async(),
            wwwRepo = resolve(grunt.config("build.www-repo"));
        
        pipe(["git log --oneline"], { cwd: wwwRepo })
            .then(function (result) {
                var buildNum = result.stdout.trim().split("\n").length;
                
                grunt.verbose.writeln("Build number " + buildNum);
                grunt.config("build.build-number", buildNum);
                
                done();
            }, function () {
                done(false);
            });
    });
    
    grunt.registerTask("build-sha", "Wrote www-repo SHA build.build-sha", function () {
        var done = this.async(),
            wwwRepo = resolve(grunt.config("build.www-repo"));
        
        pipe("git log -1", { cwd: wwwRepo })
            .then(function (result) {
                var sha = /commit (.*)/.exec(result.stdout.trim())[1];
                
                grunt.verbose.writeln("SHA " + sha);
                grunt.config("build.build-sha", sha);
                
                done();
            }, function () {
                done(false);
            });
    });
    
    // task: installer
    grunt.registerTask("installer", "Build installer", function () {
        grunt.task.run("installer-" + platform());
    });
    
    // task: installer-mac
    grunt.registerTask("installer-mac", "Build mac installer", function () {
        
    });
    
    // task: installer
    grunt.registerTask("installer", "Build win installer", function () {
        
    });
};