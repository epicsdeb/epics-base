# EPICS Base Release Procedures & Checklist

This document describes the procedures and provides a checklist of tasks
that should be performed when creating production releases of EPICS
Base.

## The Release Process

The decision to make a new release is taken during the
Core Developers bi-weekly meetings in an informal manner. The steps
detailed below were written to remind Andrew (or whoever does
the release) exactly what has to be done, since it's so easy to
miss steps.

### Roles

The following roles are used below:

**Release Manager**
Responsible for managing and tagging the release

**Core Developers**
Responsible for maintaining the EPICS software

**Website Editors**
Responsible for the EPICS websites

<table data-border="1" width="100%">
<colgroup>
<col style="width: 20%" />
<col style="width: 80%" />
</colgroup>
<tbody>
<tr>
<th>Who</th>
<th>Description</th>
</tr>
<tr>
<th colspan="3">Preparing for a release</th>
</tr>
<tr>
<td>Release Manager</td>
<td>Notify core developers about the upcoming release and ask about any
remaining tasks that must be finished.</td>
</tr>
<tr>
<td>All developers</td>
<td>Check the bug tracker for any outstanding items and handle
appropriately.</td>
</tr>
<tr>
<td>Release Manager</td>
<td>Set a Feature Freeze date, by which time all Git branches for
enhancements and new functionality should have been merged. After this
date, commits and merges should only be made to fix problems that show
up during testing.</td>
</tr>
<tr>
<td>Release Manager<br />
&amp; all developers</td>
<td>Request that documentation be updated and information about new
features be added before the release date:
<ul>
<li>Release Notes</li>
<li>Doxygen annotations</li>
<li>Other documents on
<a href="https://docs.epics-controls.org/en/latest/">
docs.epics-controls.org</a></li>
</ul></td>
</tr>
<tr>
<td>Release Manager</td>
<td>Review and update this checklist for the upcoming release. Update
the release version number in the tags and messages below.</td>
</tr>
<tr>
<td colspan="2">Testing</td>
</tr>
<tr>
<td>Platform Developers</td>
<td>Run the internal test programs on all appropriate platforms.</td>
</tr>
<tr>
<td>Platform Developers</td>
<td>Check that all makeBaseApp templates build and run properly, all
<em>xxxApp</em> and <em>xxxBoot</em> types and any internal options,
e.g. setting <code>STATIC_BUILD=YES</code> or using a different
<code>INSTALL_LOCATION</code> in configure/CONFIG_SITE.</td>
</tr>
<tr>
<td>Release Manager</td>
<td>Check that documentation has been updated:
<ul>
<li>Release Notes</li>
<li>Doxygen annotations</li>
<li>Other documents on
<a href="https://docs.epics-controls.org/en/latest/">
docs.epics-controls.org</a></li>
</ul></td>
</tr>
<tr>
<td colspan="2" id="ReleaseApproval">Release Approval</td>
</tr>
<tr>
<td>Core Developers</td>
<td>Reach a consensus that the software is ready to release.</td>
</tr>
<tr>
<td colspan="2">Creating the final release version</td>
</tr>
<tr>
<td>Release Manager</td>
<td><h4 id="for-each-external-submodule-to-be-tagged">For each external
submodule to be tagged</h4>
<ol>
<li><code>cd base-7.0/modules/&lt;module&gt;; git grep UNRELEASED</code>
and insert the submodule's version number into any doxygen annotations
that have a <code>@since&nbsp;UNRELEASED</code> comment. Commit (don't push
yet).</li>
<li>Check that the submodule's Release Notes have been updated to cover
all changes; add missing items as necessary, and set the module version
number and release date if appropriate.<br />
Commit the changes to the submodule's Notes file (don't push).</li>
<li>Copy the new submodule version number and Release Notes entries into
a new file named
<em><code>module</code></em><code>-</code><em><code>release</code></em>
in the <code>base-7.0/documentation/new-notes</code> directory.</li>
<li><p>Edit the module's release version file
<code>configure/CONFIG_</code><em><code>module</code></em><code>_VERSION</code>
and the <code>Doxyfile</code>s in the top-level and/or documentation
directories. In these, set <code>DEVELOPMENT_FLAG=0</code> and remove
<code>-dev</code> from the <code>PROJECT_NUMBER</code> string. Commit
these changes (don't push):</p>
<blockquote>
<code> git commit -m 'Final commit for &lt;submodule-version&gt;' </code>
</blockquote></li>
<li><p>Tag the submodule:</p>
<blockquote>
<code> git tag -m 'ANJ: Tag for EPICS 7.0.10' &lt;submodule-version&gt; </code>
</blockquote></li>
<li><p>Generate documentation for modules with
<code>release_notes.dox</code> files. Prepare to update the github-pages
website as follows:</p>
<blockquote>
<code> cd base-7.0/modules/&lt;module&gt;/documentation</code><br />
<code>make commit</code><br />
<code>git push --force upstream gh-pages </code>
</blockquote>
<p><em>Q: Delay this <code>git push</code> until later?</em></p></li>
<li><p>Update the Git submodule on the Base-7.0 branch to the
newly-tagged version, check the module's status matches the tag:</p>
<blockquote>
<code> cd base-7.0/modules</code><br />
<code>git add &lt;module&gt;</code><br />
<code>git submodule status --cached </code>
</blockquote>
<p>Don't commit the submodule updates yet.</p></li>
<li>Edit the module's release version file
<code>configure/CONFIG_</code><em><code>module</code></em><code>_VERSION</code>
and its top-level <code>Doxyfile</code>; increment the
<code>MAINTENANCE_VERSION</code>, set the <code>DEVELOPMENT_FLAG</code>
value to 1, and update the <code>PROJECT_NUMBER</code> string, appending
<code>-dev</code> to the new module version number. Commit changes.</li>
<li><p>Push commits and the new tag to the submodule's GitHub repository
(assumed to be the <code>upstream</code> remote):</p>
<blockquote>
<code> cd base-7.0/modules/&lt;module&gt;</code><br />
<code>git push --follow-tags upstream master </code>
</blockquote></li>
</ol></td>
</tr>
<tr>
<td>Release Manager</td>
<td><h4 id="after-all-submodules-have-been-updated">After all submodules
have been updated:</h4>
<ol>
<li><p>Commit the submodule updates which were added for each submodule
in step 7 above to the 7.0 branch (don't push):</p>
<blockquote>
<code> cd base-7.0/modules</code><br />
<code>git commit -m "Update git submodules for release" </code>
</blockquote></li>
<li>Make sure that the output from
<code>git submodule status --cached</code> only shows the appropriate
version tags in the right-most parenthesized column with no
<code>-</code><em><code>n</code></em><code>-g</code><em><code>xxxxxxx</code></em>
suffix.</li>
<li><p>Add and commit the new Release Note entry files that were created
for each submodule in step 3 above (don't push):</p>
<blockquote>
<code> cd base-7.0/documentation</code><br />
<code>git add new-notes</code><br />
<code>git commit -m "Add submodule release note entries" </code>
</blockquote></li>
</ol></td>
</tr>
<tr>
<td>Release Manager</td>
<td><h4 id="in-the-main-epics-base-repository">In the main epics-base
repository</h4>
<ol>
<li><code>cd base-7.0; git grep UNRELEASED</code> and insert the release
version to any doxygen annotations that have a
<code>@since&nbsp;UNRELEASED</code> comment. Commit (don't push).</li>
<li>Edit the main EPICS Base version file and the built-in module
version files:
<ul>
<li><code>configure/CONFIG_BASE_VERSION</code></li>
<li><code>configure/CONFIG_LIBCOM_VERSION</code></li>
<li><code>configure/CONFIG_CA_VERSION</code></li>
<li><code>configure/CONFIG_DATABASE_VERSION</code></li>
</ul></li>
<li>Version numbers should be set according to the level of changes made
since the last release. Note that the <code>MAINTENANCE_VERSION</code>
or <code>PATCH_LEVEL</code> value will have been incremented immediately
after the previous release tag was applied, so don't double-increment
them. Set all <code>DEVELOPMENT_FLAG</code> values to 0 and set the
<code>EPICS_DEV_SNAPSHOT</code> to an empty string (no quotes).</li>
<li><p>Commit the above changes (don't push):</p>
<blockquote>
<code> cd base-7.0</code><br />
<code>git add configure/CONFIG_*_VERSION</code><br />
<code>git commit -m "Set core version numbers for release" </code>
</blockquote></li>
<li><p>When <code>EPICS_DEV_SNAPSHOT</code> is empty because a release
is being created, the <code>documentation/Makefile</code> supports a
build target <code>release</code> for creating a new release notes file
<code>documentation/RELEASE-</code><em><code>version</code></em><code>.md</code>
from the Markdown files in the <code>documentation/new-notes</code>
directory. When run, it copies the notes entries from all the
<code>new-notes/*.md</code> files, then deletes the files and prepares a
Git commit to apply those changes permanently to the repository.</p>
<p>Run these commands to generate the <code>RELEASE-7.0.10.md</code>
file and remove the individual release note entry files:</p>
<blockquote>
<code> cd base-7.0/documentation</code><br />
<code>make </code>
</blockquote></li>
<li><p>The <code>make release</code> command add some changes into the
Git index but didn't commit them. These commands let you check what was
done and commit the result (don't push yet!):</p>
<blockquote>
<code> git status</code><br />
<code>git diff --staged</code><br />
<code>git commit -m "Generate RELEASE-7.0.10.md notes file" </code>
</blockquote>
<p>To undo those Git actions and confirm that happened, run these:</p>
<blockquote>
<code> make unrelease</code><br />
<code>git status </code>
</blockquote></li>
</ol></td>
</tr>
<tr>
<td>Release Manager</td>
<td>Tag the epics-base module in Git:
<blockquote>
<code> cd base-7.0</code><br />
<code>git tag -m 'ANJ: Tagged for release' R7.0.10 </code>
</blockquote>
<p>Don't push to GitHub yet.</p></td>
</tr>
<tr>
<td>Release Manager</td>
<td><h4 id="after-tagging-the-release">After tagging the release</h4>
Edit the main EPICS Base version file and the built-in module version
files:
<ul>
<li><code>configure/CONFIG_BASE_VERSION</code></li>
<li><code>configure/CONFIG_LIBCOM_VERSION</code></li>
<li><code>configure/CONFIG_CA_VERSION</code></li>
<li><code>configure/CONFIG_DATABASE_VERSION</code></li>
</ul>
<p>Version numbers should be set for the next expected patch/maintenance
release by incrementing the MAINTENANCE_VERSION or PATCH_LEVEL value in
each file. Set all <code>DEVELOPMENT_FLAG</code> values to 1 and
<code>EPICS_DEV_SNAPSHOT</code> to "-DEV".</p>
<p>Set up the headings in the Release Notes for the next release version
number and restore the warning about this being an unreleased version of
EPICS.</p>
<p>Commit these changes (don't push).</p></td>
</tr>
<tr>
<td>Release Manager</td>
<td>Export the tagged version into a tarfile. The
<code>make-tar.sh</code> script generates a gzipped tarfile directly
from the tag, excluding the files and directories that are only used for
continuous integration:
<blockquote>
<code> cd base-7.0</code><br />
<code>./.tools/make-tar.sh R7.0.10 ../base-7.0.10.tar.gz base-7.0.10/ </code>
</blockquote>
Create a GPG signature file of the tarfile as follows:
<blockquote>
<code> cd ..</code><br />
<code>gpg --armor --sign --detach-sig base-7.0.10.tar.gz </code>
</blockquote></td>
</tr>
<tr>
<td>Release Manager</td>
<td>Test the tar file by extracting its contents and building it on at
least one supported platform. If this succeeds the commits and new git
tag can be pushed to the GitHub repository's 7.0 branch (assumed to be
the <code>upstream</code> remote):
<blockquote>
<code> git push --follow-tags upstream 7.0 </code>
</blockquote></td>
</tr>
<tr>
<td colspan="2">Publish to epics.anl.gov</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Copy the tarfile and its signature to the Base download area of the
website.</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Add the new release tar file to the website Base download index
page.</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Create or update the website subdirectory that holds the release
documentation, and copy in the files to be published with this release
version.</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Update the webpage for the new release with links to the release
documents and tar file.</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Link to the release webpage from other relevant areas of the website
- update front page and sidebars.</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Add an entry to the website News page, linking to the new version
webpage.</td>
</tr>
<tr>
<td colspan="2">Publish to epics-controls.org</td>
</tr>
<tr>
<td>Website Editor</td>
<td>Upload the tar file and its <code>.asc</code> signature file to the
epics-controls web-server.
<blockquote>
<code> scp base-7.0.10.tar.gz base-7.0.10.tar.gz.asc epics-controls:download/base</code><br />
</blockquote></td>
</tr>
<tr>
<td>Website Editor</td>
<td><p>Follow instructions on
<a href="https://epics-controls.org/resources-and-support/documents/epics-website-documentation/adding-a-page-for-a-new-release/">
Add a page for a new release</a>
to create a new release webpage (not
required for a patch release, just edit the existing page). Update the
TablePress "Point Releases" table and add the new download, and adjust
the Html Snippet for the series download.</p>
<p>Not covered in those instructions: Go to Posts, find a previous
release and use "Duplicate Post", then edit the result and publish it.
This generates the News item.</p></td>
</tr>
<tr>
<td colspan="2">Publish to GitHub</td>
</tr>
<tr>
<td>Release Manager</td>
<td>Go to the GitHub
<a href="https://github.com/epics-base/epics-base/releases/new?tag=R7.0.10">
Create
release from tag R7.0.10</a> page. Upload the tar file and its
<code>.asc</code> signature file to the new GitHub release page, or just
drag-n-drop them into the page. Copy/paste the text from the previous
release and edit. Submit.</td>
</tr>
<tr>
<td>Release Manager</td>
<td>We used to close out bug reports in Launchpad at release-time, this
would be the time to do that if we have an equivalent on GitHub.</td>
</tr>
<tr>
<td colspan="2">Make Announcement</td>
</tr>
<tr>
<td>Release Manager</td>
<td>Announce the release on the tech-talk mailing list.</td>
</tr>
</tbody>
</table>
